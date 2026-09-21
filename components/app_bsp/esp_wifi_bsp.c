#include <stdio.h>
#include <string.h>
#include "esp_wifi_bsp.h"
#include "wifi_portal.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "esp_mac.h"

static const char *TAG = "wifi_bsp";

EventGroupHandle_t wifi_even_ = NULL;
esp_bsp_t user_esp_bsp;
char espwifi_cfg_ssid[33] = {0};
char espwifi_cfg_pass[65] = {0};

#define SCAN_MAX_AP 16
static wifi_ap_record_t s_scan_list[SCAN_MAX_AP];
static uint16_t s_scan_count = 0;

static esp_netif_t *s_netif_sta = NULL;
static esp_netif_t *s_netif_ap = NULL;
static bool s_netif_ready = false;
static bool s_event_loop = false;
static bool s_wifi_ready = false;   /* driver initialised */
static bool s_wifi_started = false; /* esp_wifi_start() done */
static bool s_config_mode = false;
static bool s_connect_on_start = false;
static SemaphoreHandle_t s_connect_sem = NULL;

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

static void base_init(void)
{
    if (wifi_even_ == NULL) {
        wifi_even_ = xEventGroupCreate();
        memset(&user_esp_bsp, 0, sizeof(esp_bsp_t));
    }
    if (s_connect_sem == NULL) {
        s_connect_sem = xSemaphoreCreateBinary();
    }
    nvs_flash_init();
    if (!s_netif_ready) {
        esp_netif_init();
        s_netif_ready = true;
    }
    if (!s_event_loop) {
        esp_event_loop_create_default();
        s_event_loop = true;
    }
}

static void wifi_driver_init(void)
{
    base_init();
    if (s_wifi_ready) {
        return;
    }
    s_netif_sta = esp_netif_create_default_wifi_sta();
    s_netif_ap = esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_event_handler_instance_t inst_wifi;
    esp_event_handler_instance_t inst_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &inst_wifi);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &inst_ip);
    s_wifi_ready = true;
}

static void save_credentials(const char *ssid, const char *pass)
{
    nvs_handle_t h;
    if (nvs_open("wificfg", NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_str(h, "ssid", ssid);
        nvs_set_str(h, "pass", pass);
        nvs_commit(h);
        nvs_close(h);
    }
}

bool espwifi_is_active(void)
{
    return s_wifi_started;
}

bool espwifi_connect_stored(void)
{
    char ssid[33] = {0};
    char pass[65] = {0};
    nvs_handle_t h;
    bool found = false;

    base_init();
    if (nvs_open("wificfg", NVS_READONLY, &h) == ESP_OK) {
        size_t len = sizeof(ssid);
        found = (nvs_get_str(h, "ssid", ssid, &len) == ESP_OK && len > 1);
        len = sizeof(pass);
        if (nvs_get_str(h, "pass", pass, &len) != ESP_OK) {
            pass[0] = 0;
        }
        nvs_close(h);
    }
    if (!found) {
        ESP_LOGI(TAG, "no stored credentials, skipping Wi-Fi");
        return false;
    }

    wifi_driver_init();
    wifi_config_t wifi_config = {0};
    memcpy(wifi_config.sta.ssid, ssid, strnlen(ssid, sizeof(wifi_config.sta.ssid)));
    memcpy(wifi_config.sta.password, pass, strnlen(pass, sizeof(wifi_config.sta.password)));
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    s_connect_on_start = true;
    esp_wifi_start();
    s_wifi_started = true;
    return true;
}

void espwifi_config_start(void)
{
    wifi_driver_init();
    xEventGroupClearBits(wifi_even_, WIFI_EV_CREDENTIALS | WIFI_EV_STA_CONNECTED |
                         WIFI_EV_STA_FAILED | WIFI_EV_AP_CLIENT);
    s_config_mode = true;
    s_scan_count = 0;

    wifi_config_t ap_config = {0};
    strcpy((char *)ap_config.ap.ssid, ESPWIFI_AP_SSID);
    ap_config.ap.ssid_len = strlen(ESPWIFI_AP_SSID);
    ap_config.ap.channel = 1;
    ap_config.ap.max_connection = 4;
    strcpy((char *)ap_config.ap.password, ESPWIFI_AP_PASS);
    ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;

    esp_wifi_set_mode(WIFI_MODE_APSTA);
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    esp_wifi_start();
    s_wifi_started = true;

    /* Scan as STA; wait for STA start first. */
    xEventGroupWaitBits(wifi_even_, WIFI_EV_STA_START, pdTRUE, pdTRUE, pdMS_TO_TICKS(10000));
    if (esp_wifi_scan_start(NULL, true) == ESP_OK) {
        uint16_t num = SCAN_MAX_AP;
        if (esp_wifi_scan_get_ap_records(&num, s_scan_list) == ESP_OK) {
            s_scan_count = num;
        }
    }
    user_esp_bsp.apNum = s_scan_count;
    xEventGroupSetBits(wifi_even_, WIFI_EV_SCAN_DONE);
    ESP_LOGI(TAG, "config mode: %u APs scanned, portal starting", s_scan_count);

    wifi_portal_start();
}

bool espwifi_config_connect(const char *ssid, const char *pass)
{
    wifi_config_t wifi_config = {0};
    memcpy(wifi_config.sta.ssid, ssid, strnlen(ssid, sizeof(wifi_config.sta.ssid)));
    memcpy(wifi_config.sta.password, pass, strnlen(pass, sizeof(wifi_config.sta.password)));

    xEventGroupClearBits(wifi_even_, WIFI_EV_STA_CONNECTED | WIFI_EV_STA_FAILED);
    s_connect_on_start = false;
    esp_wifi_set_mode(WIFI_MODE_APSTA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_connect();

    bool ok = (xSemaphoreTake(s_connect_sem, pdMS_TO_TICKS(20000)) == pdTRUE)
              && (xEventGroupGetBits(wifi_even_) & WIFI_EV_STA_CONNECTED);
    if (ok) {
        save_credentials(ssid, pass);
    }
    return ok;
}

void espwifi_config_stop(void)
{
    wifi_portal_stop();
    s_config_mode = false;
    if (xEventGroupGetBits(wifi_even_) & WIFI_EV_STA_CONNECTED) {
        /* Keep the working STA connection, drop the hotspot. */
        esp_wifi_set_mode(WIFI_MODE_STA);
        ESP_LOGI(TAG, "config done, STA kept up");
    } else {
        espwifi_deinit();
    }
}

const wifi_ap_record_t *espwifi_get_scan_results(uint16_t *count)
{
    *count = s_scan_count;
    return s_scan_list;
}

void espwifi_deinit(void)
{
    wifi_portal_stop();
    if (s_wifi_ready) {
        esp_wifi_stop();
        esp_wifi_deinit();
        esp_netif_destroy_default_wifi(s_netif_sta);
        esp_netif_destroy_default_wifi(s_netif_ap);
        s_netif_sta = NULL;
        s_netif_ap = NULL;
        s_wifi_ready = false;
        s_wifi_started = false;
        s_connect_on_start = false;
    }
    if (s_event_loop) {
        esp_event_loop_delete_default();
        s_event_loop = false;
    }
}

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            xEventGroupSetBits(wifi_even_, WIFI_EV_STA_START);
            if (s_connect_on_start) {
                esp_wifi_connect();
            }
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            if (s_config_mode) {
                wifi_event_sta_disconnected_t *ev = event_data;
                if (ev->reason != WIFI_REASON_ASSOC_LEAVE) {
                    xEventGroupSetBits(wifi_even_, WIFI_EV_STA_FAILED);
                    xSemaphoreGive(s_connect_sem);
                }
            }
            break;
        case WIFI_EVENT_AP_STACONNECTED:
            xEventGroupSetBits(wifi_even_, WIFI_EV_AP_CLIENT);
            break;
        default:
            break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        uint32_t pxip = event->ip_info.ip.addr;
        snprintf(user_esp_bsp._ip, sizeof(user_esp_bsp._ip),
                 "%d.%d.%d.%d", (uint8_t)(pxip), (uint8_t)(pxip >> 8),
                 (uint8_t)(pxip >> 16), (uint8_t)(pxip >> 24));
        ESP_LOGI(TAG, "got IP %s", user_esp_bsp._ip);
        if (s_config_mode) {
            xEventGroupSetBits(wifi_even_, WIFI_EV_STA_CONNECTED);
            xEventGroupClearBits(wifi_even_, WIFI_EV_STA_FAILED);
            xSemaphoreGive(s_connect_sem);
        }
    }
}
