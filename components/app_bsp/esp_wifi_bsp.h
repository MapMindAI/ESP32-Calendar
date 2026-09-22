#ifndef ESP_WIFI_BSP_H
#define ESP_WIFI_BSP_H

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"

extern EventGroupHandle_t wifi_even_;

/* wifi_even_ bits */
#define WIFI_EV_STA_START      0x01
#define WIFI_EV_SCAN_DONE      0x02
#define WIFI_EV_CREDENTIALS    0x04  /* portal received ssid/pass (see espwifi_cfg_ssid/pass) */
#define WIFI_EV_STA_CONNECTED  0x08  /* STA associated and got IP during config */
#define WIFI_EV_STA_FAILED     0x10  /* STA connect attempt failed during config */
#define WIFI_EV_AP_CLIENT      0x20  /* a station joined the config softAP */

typedef struct
{
  char _ip[25];
  int8_t rssi;
  int8_t apNum;
}esp_bsp_t;
extern esp_bsp_t user_esp_bsp;

/* Filled by the captive portal when the user submits the form. */
extern char espwifi_cfg_ssid[33];
extern char espwifi_cfg_pass[65];

/* Name and WPA2 passphrase of the configuration hotspot. The passphrase is shown
   on the setup screen, so it is a fixed, readable string rather than a secret. */
#define ESPWIFI_AP_SSID "ESP32-Calendar"
#define ESPWIFI_AP_PASS "calendar"

#ifdef __cplusplus
extern "C" {
#endif

/* Tear down Wi-Fi: portal, AP and driver. Caller must check espwifi_is_active(). */
void espwifi_deinit(void);

/* True if Wi-Fi (STA) is currently initialised and started. */
bool espwifi_is_active(void);

/* Sync path: read credentials from NVS; if present start STA and connect.
   Returns true when credentials existed and Wi-Fi was started. The radio stays
   up until the caller calls espwifi_deinit(). */
bool espwifi_connect_stored(void);

/* Block until the station has an address or `timeout_ms` elapses. Pairs with
   espwifi_connect_stored(). */
bool espwifi_wait_for_ip(uint32_t timeout_ms);

/* Config path: start softAP + captive portal, scan and keep results for the portal. */
void espwifi_config_start(void);

/* Config path: stop the portal and tear Wi-Fi down. The radio is not kept up
   after setup — the daily sync window in user_app brings it back when needed. */
void espwifi_config_stop(void);

/* Use the submitted credentials to connect. Blocks up to 20 s.
   On success saves credentials to NVS. Returns true on GOT_IP. */
bool espwifi_config_connect(const char *ssid, const char *pass);

/* Scan results kept for the portal page (valid after espwifi_config_start). */
const wifi_ap_record_t *espwifi_get_scan_results(uint16_t *count);

#ifdef __cplusplus
}
#endif

#endif
