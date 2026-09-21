#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <lwip/sockets.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_wifi_bsp.h"

static const char *TAG = "wifi_portal";

#define AP_IP "192.168.4.1"
#define DNS_PORT 53

static httpd_handle_t s_httpd = NULL;
static TaskHandle_t s_dns_task = NULL;
static volatile bool s_dns_run = false;

/* ------------------------------------------------------------------ */
/* DNS redirect: answer every A query with 192.168.4.1                 */
/* ------------------------------------------------------------------ */

static void dns_server_task(void *arg)
{
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        ESP_LOGE(TAG, "dns socket failed");
        vTaskDelete(NULL);
    }
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DNS_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        ESP_LOGE(TAG, "dns bind failed");
        close(fd);
        vTaskDelete(NULL);
    }

    /* Static: this task is a singleton and 1 KB of buffers does not fit on its stack. */
    static uint8_t req[512];
    static uint8_t reply[512];
    while (s_dns_run) {
        struct sockaddr_in src = {0};
        socklen_t slen = sizeof(src);
        ssize_t len = recvfrom(fd, req, sizeof(req), 0, (struct sockaddr *)&src, &slen);
        if (len < 12 || !s_dns_run) {
            continue;
        }
        uint16_t qdcount = (req[4] << 8) | req[5];
        if (qdcount == 0) {
            continue;
        }

        /* Build the reply: header + the original question + one answer. */
        size_t off = 0;
        memcpy(reply, req, 2);            /* ID */
        reply[2] = 0x81; reply[3] = 0x80; /* standard response, no error */
        reply[4] = req[4]; reply[5] = req[5]; /* QDCOUNT */
        reply[6] = 0; reply[7] = 1;       /* ANCOUNT = 1 */
        reply[8] = reply[9] = reply[10] = reply[11] = 0;
        off = 12;
        /* copy the question section(s) verbatim */
        size_t qend = 12;
        for (uint16_t q = 0; q < qdcount; q++) {
            while (qend < (size_t)len && req[qend] != 0) {
                qend += 1 + req[qend];
            }
            qend += 5; /* null label + QTYPE + QCLASS */
        }
        if (qend > (size_t)len) {
            qend = len;
        }
        if (qend + 16 > sizeof(reply)) {
            continue; /* question section leaves no room for the answer record */
        }
        memcpy(reply + off, req + off, qend - off);
        off = qend;
        /* answer: pointer to the name, type A, class IN, TTL 300, 4-byte IP */
        reply[off++] = 0xC0; reply[off++] = 0x0C;
        reply[off++] = 0; reply[off++] = 1;
        reply[off++] = 0; reply[off++] = 1;
        reply[off++] = 0; reply[off++] = 0; reply[off++] = 0x01; reply[off++] = 0x2C;
        reply[off++] = 0; reply[off++] = 4;
        inet_pton(AF_INET, AP_IP, reply + off);
        off += 4;
        sendto(fd, reply, off, 0, (struct sockaddr *)&src, slen);
    }
    close(fd);
    vTaskDelete(NULL);
}

/* ------------------------------------------------------------------ */
/* HTTP server                                                         */
/* ------------------------------------------------------------------ */

static void html_escape(char *dst, size_t dst_size, const char *src, size_t src_len)
{
    size_t o = 0;
    for (size_t i = 0; i < src_len && o + 1 < dst_size; i++) {
        char c = src[i];
        if (c == '<') {
            if (o + 4 >= dst_size) break;
            memcpy(dst + o, "&lt;", 4); o += 4;
        } else if (c == '>') {
            if (o + 4 >= dst_size) break;
            memcpy(dst + o, "&gt;", 4); o += 4;
        } else if (c == '&') {
            if (o + 5 >= dst_size) break;
            memcpy(dst + o, "&amp;", 5); o += 5;
        } else if (c == '"') {
            if (o + 6 >= dst_size) break;
            memcpy(dst + o, "&quot;", 6); o += 6;
        } else {
            dst[o++] = c;
        }
    }
    dst[o] = 0;
}

static void url_decode(char *s)
{
    char *r = s, *w = s;
    while (*r) {
        if (*r == '+') {
            *w++ = ' ';
            r++;
        } else if (*r == '%' && r[1] && r[2]) {
            char hex[3] = {r[1], r[2], 0};
            *w++ = (char)strtol(hex, NULL, 16);
            r += 3;
        } else {
            *w++ = *r++;
        }
    }
    *w = 0;
}

/* Find a form field in "ssid=..&pass=.." (already url-decoded as a whole is
   NOT done; we decode each field value separately). */
static bool form_get_field(const char *body, const char *field, char *out, size_t out_size)
{
    size_t flen = strlen(field);
    const char *p = body;
    while ((p = strstr(p, field)) != NULL) {
        if (p[flen] == '=' && (p == body || p[-1] == '&')) {
            p += flen + 1;
            const char *end = strchr(p, '&');
            size_t vlen = end ? (size_t)(end - p) : strlen(p);
            if (vlen >= out_size) vlen = out_size - 1;
            memcpy(out, p, vlen);
            out[vlen] = 0;
            url_decode(out);
            return true;
        }
        p += flen;
    }
    return false;
}

static esp_err_t handler_index(httpd_req_t *req)
{
    uint16_t count = 0;
    const wifi_ap_record_t *aps = espwifi_get_scan_results(&count);

    size_t buf_size = 4096;
    char *buf = malloc(buf_size);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "oom");
        return ESP_FAIL;
    }
    size_t off = snprintf(buf, buf_size,
        "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>Wi-Fi Setup</title></head><body>"
        "<h2>ESP32-Calendar Wi-Fi Setup</h2>"
        "<form action=\"/connect\" method=\"POST\">"
        "Wi-Fi:<br><select name=\"ssid\" style=\"width:100%%;font-size:18px\">");
    for (uint16_t i = 0; i < count && off < buf_size - 80; i++) {
        char ssid[33 * 6 + 1];
        html_escape(ssid, sizeof(ssid), (const char *)aps[i].ssid, strnlen((const char *)aps[i].ssid, 32));
        off += snprintf(buf + off, buf_size - off, "<option value=\"%s\">%s</option>", ssid, ssid);
    }
    off += snprintf(buf + off, buf_size - off,
        "</select><br><br>Password:<br>"
        "<input type=\"password\" name=\"pass\" maxlength=\"63\" style=\"width:100%%;font-size:18px\"><br><br>"
        "<input type=\"submit\" value=\"Connect\" style=\"width:100%%;font-size:20px;padding:10px\">"
        "</form></body></html>");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, buf, off);
    free(buf);
    return ESP_OK;
}

static esp_err_t handler_connect(httpd_req_t *req)
{
    char body[256] = {0};
    int len = httpd_req_recv(req, body, sizeof(body) - 1);
    if (len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "empty");
        return ESP_FAIL;
    }
    char ssid[33] = {0};
    char pass[65] = {0};
    if (!form_get_field(body, "ssid", ssid, sizeof(ssid))) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing ssid");
        return ESP_FAIL;
    }
    form_get_field(body, "pass", pass, sizeof(pass));
    strncpy(espwifi_cfg_ssid, ssid, sizeof(espwifi_cfg_ssid) - 1);
    strncpy(espwifi_cfg_pass, pass, sizeof(espwifi_cfg_pass) - 1);
    xEventGroupSetBits(wifi_even_, WIFI_EV_CREDENTIALS);
    ESP_LOGI(TAG, "credentials received for '%s'", espwifi_cfg_ssid);

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req,
        "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
        "<meta http-equiv=\"refresh\" content=\"2;url=/status\"></head>"
        "<body><h2>Connecting...</h2></body></html>", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t handler_status(httpd_req_t *req)
{
    EventBits_t bits = xEventGroupGetBits(wifi_even_);
    const char *page;
    if (bits & WIFI_EV_STA_CONNECTED) {
        page = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"></head><body>"
               "<h2>Connected</h2><p>IP: %s</p>"
               "<p>Settings saved. You can close this page.</p></body></html>";
        char buf[256];
        snprintf(buf, sizeof(buf), page, user_esp_bsp._ip);
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req, buf, HTTPD_RESP_USE_STRLEN);
    } else if (bits & WIFI_EV_STA_FAILED) {
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req,
            "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
            "<meta http-equiv=\"refresh\" content=\"3;url=/\"></head>"
            "<body><h2>Failed</h2><p>Retrying...</p></body></html>",
            HTTPD_RESP_USE_STRLEN);
    } else {
        httpd_resp_set_type(req, "text/html");
        httpd_resp_send(req,
            "<!DOCTYPE html><html><head><meta charset=\"utf-8\">"
            "<meta http-equiv=\"refresh\" content=\"2\"></head>"
            "<body><h2>Connecting...</h2></body></html>", HTTPD_RESP_USE_STRLEN);
    }
    return ESP_OK;
}

static esp_err_t handler_redirect(httpd_req_t *req)
{
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

void wifi_portal_start(void)
{
    if (s_dns_task == NULL) {
        s_dns_run = true;
        xTaskCreatePinnedToCore(dns_server_task, "dns_redirect", 4096, NULL, 3,
                                &s_dns_task, 0);
    }
    if (s_httpd == NULL) {
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.max_uri_handlers = 8;
        /* Phone apps probing the portal send long headers; 1 KB triggers 431. */
        config.max_req_hdr_len = 2048;
        config.uri_match_fn = httpd_uri_match_wildcard;
        if (httpd_start(&s_httpd, &config) == ESP_OK) {
            httpd_uri_t uri_index = {.uri = "/", .method = HTTP_GET, .handler = handler_index};
            httpd_uri_t uri_connect = {.uri = "/connect", .method = HTTP_POST, .handler = handler_connect};
            httpd_uri_t uri_status = {.uri = "/status", .method = HTTP_GET, .handler = handler_status};
            httpd_uri_t uri_catch = {.uri = "/*", .method = HTTP_GET, .handler = handler_redirect};
            httpd_register_uri_handler(s_httpd, &uri_index);
            httpd_register_uri_handler(s_httpd, &uri_connect);
            httpd_register_uri_handler(s_httpd, &uri_status);
            httpd_register_uri_handler(s_httpd, &uri_catch);
            ESP_LOGI(TAG, "captive portal on http://" AP_IP);
        }
    }
}

void wifi_portal_stop(void)
{
    if (s_httpd != NULL) {
        httpd_stop(s_httpd);
        s_httpd = NULL;
    }
    if (s_dns_task != NULL) {
        s_dns_run = false;
        /* Wake the recvfrom() so the task can exit. */
        int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (fd >= 0) {
            struct sockaddr_in dst = {0};
            dst.sin_family = AF_INET;
            dst.sin_port = htons(DNS_PORT);
            dst.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            sendto(fd, "x", 1, 0, (struct sockaddr *)&dst, sizeof(dst));
            close(fd);
        }
        s_dns_task = NULL;
    }
}
