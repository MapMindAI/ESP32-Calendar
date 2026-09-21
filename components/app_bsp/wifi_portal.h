#ifndef WIFI_PORTAL_H
#define WIFI_PORTAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Start the captive portal: UDP DNS redirect (any domain -> 192.168.4.1)
   plus an HTTP server serving the Wi-Fi setup page. Uses the scan list
   from esp_wifi_bsp.h. Safe to call when already running. */
void wifi_portal_start(void);

/* Stop the DNS task and the HTTP server. Safe to call when not running. */
void wifi_portal_stop(void);

#ifdef __cplusplus
}
#endif

#endif
