#include <sys/time.h>

#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <esp_netif_sntp.h>
#include <esp_sntp.h>

#include "time_manager.h"
#include "i2c_equipment.h"

static const char *TAG = "time_mgr";

/* Anything before this is the epoch leaking through, not a date a user set. */
#define TIME_VALID_MIN_YEAR 2024

static void write_rtc_from_system_time(void)
{
    struct tm local;
    time_t    now = 0;

    time(&now);
    localtime_r(&now, &local);
    Rtc_SetTime(local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
                local.tm_hour, local.tm_min, local.tm_sec);
    ESP_LOGI(TAG, "RTC set from SNTP: %04d-%02d-%02d %02d:%02d:%02d",
             local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
             local.tm_hour, local.tm_min, local.tm_sec);
}

esp_err_t time_manager_init(I2cMasterBus *bus)
{
    rtcTimeStruct_t rtc_time;
    struct tm       local = {};
    struct timeval  tv;

    if (bus == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    setenv("TZ", CONFIG_CALENDAR_TIMEZONE, 1);
    tzset();

    Rtc_Setup(bus, 0x51);
    Rtc_GetTime(&rtc_time);

    if (rtc_time.year < TIME_VALID_MIN_YEAR || rtc_time.year > 2099) {
        ESP_LOGW(TAG, "RTC holds %04d, waiting for SNTP", rtc_time.year);
        return ESP_ERR_INVALID_STATE;
    }

    local.tm_year  = rtc_time.year - 1900;
    local.tm_mon   = rtc_time.month - 1;
    local.tm_mday  = rtc_time.day;
    local.tm_hour  = rtc_time.hour;
    local.tm_min   = rtc_time.minute;
    local.tm_sec   = rtc_time.second;
    local.tm_isdst = -1;

    tv.tv_sec  = mktime(&local);
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
    ESP_LOGI(TAG, "system clock seeded from RTC: %04d-%02d-%02d %02d:%02d:%02d",
             rtc_time.year, rtc_time.month, rtc_time.day,
             rtc_time.hour, rtc_time.minute, rtc_time.second);
    return ESP_OK;
}

bool time_manager_sync_now(uint32_t timeout_ms)
{
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_CALENDAR_NTP_SERVER);
    config.start             = true;

    esp_err_t err = esp_netif_sntp_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_netif_sntp_init: %s", esp_err_to_name(err));
        return false;
    }

    /* Block rather than use a sync callback: the caller shuts the radio down the
       moment this returns, so it has to know whether the answer arrived. */
    bool synced = (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(timeout_ms)) == ESP_OK);
    esp_netif_sntp_deinit();

    if (synced) {
        write_rtc_from_system_time();
    } else {
        ESP_LOGW(TAG, "no answer from %s within %u ms", CONFIG_CALENDAR_NTP_SERVER,
                 (unsigned)timeout_ms);
    }
    return synced;
}

bool time_manager_get_local(struct tm *out)
{
    struct tm local;
    time_t    now = 0;

    if (out == NULL) {
        return false;
    }
    time(&now);
    localtime_r(&now, &local);
    if ((local.tm_year + 1900) < TIME_VALID_MIN_YEAR) {
        return false;
    }
    *out = local;
    return true;
}
