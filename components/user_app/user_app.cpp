#include "user_app.h"
#include <assert.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <math.h>
#include <stdio.h>
#include "adc_bsp.h"
#include "button_bsp.h"
#include "calendar_calc.h"
#include "calendar_ui.h"
#include "esp_wifi_bsp.h"
#include "i2c_bsp.h"
#include "lvgl_bsp.h"
#include "sensor_manager.h"
#include "time_manager.h"
#include "wifi_setup_page.h"

static wifi_setup_page_t init_ui;
I2cMasterBus I2cbus(14,13,0);
EventGroupHandle_t ConfigGroups;

/* ConfigGroups bits */
#define CFG_REQ_START 0x01  /* Wi-Fi setup view opened */
#define CFG_REQ_STOP 0x02   /* Wi-Fi setup view closed */
#define CFG_REQ_ENABLE 0x04 /* start the setup hotspot and captive portal */
#define CFG_SYNC_NOW 0x08   /* cut Time_SyncTask's wait short and open a window now */

/* Wi-Fi is by far the largest draw on this board and the PCF85063 holds the
   clock to a few seconds a day, so the radio is down except inside a sync
   window: connect, take the time from SNTP, tear Wi-Fi back down. One window
   runs at boot, one a day at the local time below, and one after the setup view
   closes. Change TIME_SYNC_HOUR/TIME_SYNC_MINUTE to move the daily window. */
#define TIME_SYNC_HOUR            3
#define TIME_SYNC_MINUTE          30
#define TIME_SYNC_IP_TIMEOUT_MS   (20 * 1000)
#define TIME_SYNC_SNTP_TIMEOUT_MS (15 * 1000)
/* A window that came back empty is retried on this cadence instead of waiting
   out the day — a board whose RTC is flat has no time to display until one lands. */
#define TIME_SYNC_RETRY_MINUTES   30

/* The panel repaints in full on any invalidation, so a widget is only written
   when the value behind it actually moved: the clock at minute resolution, the
   environment row on a threshold, everything else when the date rolls over. */
#define SENSOR_READ_INTERVAL_MS      60000
#define TEMPERATURE_REFRESH_THRESHOLD 0.2f
#define HUMIDITY_REFRESH_THRESHOLD 1.0f
#define BATTERY_READ_INTERVAL_MS       60000

static bool is_CfgViewOn = false;

/* One user of the radio at a time: the daily sync window and the setup view both
   bring Wi-Fi up and take it down again. */
static SemaphoreHandle_t WifiMutex;

/* Clock and calendar. Polls the system clock once a second and writes a widget
   only on a minute or date change. */
void Calendar_LoopTask(void *arg) {
    environment_data_t environment;
    struct tm local;
    int last_year = -1;
    int last_month = -1;
    int last_minute = -1;
    int last_day = -1;
    bool last_valid = false;
#if LVGL_DEBUG_LOG
    uint32_t last_uptime_minutes = UINT32_MAX;
#endif
    for(;;) {
#if LVGL_DEBUG_LOG
        uint32_t uptime_minutes = (uint32_t)(esp_timer_get_time() / (60LL * 1000000LL));
        if(uptime_minutes != last_uptime_minutes && Lvgl_lock(-1)) {
            calendar_ui_update_uptime(uptime_minutes);
            Lvgl_unlock();
            Lvgl_RequestRender(11);
            last_uptime_minutes = uptime_minutes;
        }
#endif
        bool valid = time_manager_get_local(&local);
        if(!valid) {
            last_valid = false;
            last_year = -1;
            last_month = -1;
            last_minute = -1;
            last_day = -1;
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        if(!last_valid || local.tm_year != last_year || local.tm_mon != last_month ||
           local.tm_mday != last_day) {
            calendar_ui_data_t data = {};

            calendar_calc_fill(&data,&local);
            sensor_manager_last(&environment);
            data.temperature_c = environment.temperature_c;
            data.humidity_percent = environment.humidity_percent;
            data.environment_valid = environment.valid;
            if(Lvgl_lock(-1)) {
                calendar_ui_refresh_all(&data);
                Lvgl_unlock();
                Lvgl_RequestRender(1);
            }
            last_valid = true;
            last_year = local.tm_year;
            last_month = local.tm_mon;
            last_day = local.tm_mday;
            last_minute = local.tm_min;
        } else if(local.tm_min != last_minute) {
            if(Lvgl_lock(-1)) {
                calendar_ui_update_time(local.tm_hour, local.tm_min, true);
                Lvgl_unlock();
                Lvgl_RequestRender(2);
            }
            last_minute = local.tm_min;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void Battery_LoopTask(void *arg) {
    int shown_level = -1;

    for(;;) {
        uint8_t level = Adc_GetBatteryLevel();

        if(level != shown_level && Lvgl_lock(-1)) {
            calendar_ui_update_battery(level);
            Lvgl_unlock();
            Lvgl_RequestRender(3);
            shown_level = level;
        }
        vTaskDelay(pdMS_TO_TICKS(BATTERY_READ_INTERVAL_MS));
    }
}

/* Temperature and humidity. Reads once a minute; repaints only when the
   displayed value changes enough to cross a threshold. */
void Sensor_LoopTask(void *arg) {
    environment_data_t environment;
    float shown_temperature = 0.0f;
    float shown_humidity = 0.0f;
    bool shown_valid = false;
    for(;;) {
        sensor_manager_read(&environment);
        bool publish = false;
        if(environment.valid != shown_valid) {
            publish = true;
        } else if(environment.valid &&
                  (fabsf(environment.temperature_c - shown_temperature) >= TEMPERATURE_REFRESH_THRESHOLD ||
                   fabsf(environment.humidity_percent - shown_humidity) >= HUMIDITY_REFRESH_THRESHOLD)) {
          publish = true;
        }
        if(publish) {
            if(Lvgl_lock(-1)) {
                calendar_ui_update_environment(environment.temperature_c,environment.humidity_percent,environment.valid);
                Lvgl_unlock();
                Lvgl_RequestRender(4);
            }
            shown_temperature = environment.temperature_c;
            shown_humidity = environment.humidity_percent;
            shown_valid = environment.valid;
        }
        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
    }
}

static void wifi_status_publish(calendar_wifi_state_t state) {
  static calendar_wifi_state_t shown_state = CALENDAR_WIFI_UNSET;

  if (state == shown_state) {
    return;
  }
    if(Lvgl_lock(-1)) {
        calendar_ui_update_wifi(state);
        Lvgl_unlock();
        Lvgl_RequestRender(5);
        shown_state = state;
    }
}

/* One sync window: radio up, SNTP, radio down. Returns true when the clock was
   corrected. The icon follows the window, so the panel says what the radio is
   doing without the user having to open the setup view. */
static bool time_sync_window(void) {
    bool synced = false;

    xSemaphoreTake(WifiMutex,portMAX_DELAY);
    wifi_status_publish(CALENDAR_WIFI_ACTIVE);
    if(!espwifi_connect_stored()) {
        wifi_status_publish(CALENDAR_WIFI_UNSET);
        xSemaphoreGive(WifiMutex);
        return false;
    }
    if(espwifi_wait_for_ip(TIME_SYNC_IP_TIMEOUT_MS)) {
        synced = time_manager_sync_now(TIME_SYNC_SNTP_TIMEOUT_MS);
    }
    espwifi_deinit();
    wifi_status_publish(synced ? CALENDAR_WIFI_SYNCED : CALENDAR_WIFI_FAILED);
    xSemaphoreGive(WifiMutex);
    return synced;
}

/* Seconds until the next TIME_SYNC_HOUR:TIME_SYNC_MINUTE. A window that failed,
   or a clock that cannot say what time it is, gets the short retry instead. */
static uint32_t seconds_until_next_window(bool synced) {
    struct tm local;

    if(!synced || !time_manager_get_local(&local)) {
        return TIME_SYNC_RETRY_MINUTES * 60;
    }
    int now_second  = local.tm_hour * 3600 + local.tm_min * 60 + local.tm_sec;
    int next_second = TIME_SYNC_HOUR * 3600 + TIME_SYNC_MINUTE * 60;
    int remaining   = next_second - now_second;

    if(remaining <= 0) {
        remaining += 24 * 3600;
    }
    return (uint32_t)remaining;
}

/* Opens a sync window at boot and one a day after that; CFG_SYNC_NOW from the
   setup view brings the next one forward. */
void Time_SyncTask(void *arg) {
    for(;;) {
        bool synced = time_sync_window();
        uint32_t wait_s = seconds_until_next_window(synced);

        xEventGroupWaitBits(ConfigGroups,CFG_SYNC_NOW,pdTRUE,pdFALSE,pdMS_TO_TICKS(wait_s * 1000U));
    }
}

static void show_view(lv_obj_t *view)
{
    lv_obj_add_flag(calendar_ui_root(), LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(init_ui.screen_cont_wifi_setup, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(view, LV_OBJ_FLAG_HIDDEN);
}

void BOOT_LoopTask(void *arg) {
    for(;;) {
        EventBits_t even = xEventGroupWaitBits(BootButtonGroups,(0x01 | 0x02 | 0x04),pdTRUE,pdFALSE,pdMS_TO_TICKS(2000));
        if(even & 0x04) {
            if(0 == is_CfgViewOn) {
                is_CfgViewOn = 1;
                if(Lvgl_lock(-1)) {
                  show_view(init_ui.screen_cont_wifi_setup);
                  Lvgl_unlock();
                  Lvgl_RequestRender(6);
                }
                xEventGroupSetBits(ConfigGroups,CFG_REQ_START);
            } else {
                is_CfgViewOn = 0;
                if(Lvgl_lock(-1)) {
                    show_view(calendar_ui_root());
                    Lvgl_unlock();
                    Lvgl_RequestRender(7);
                }
                xEventGroupSetBits(ConfigGroups,CFG_REQ_STOP);
            }
        }
    }
}

/* KEY gestures are dispatched by the visible page. Keep these entry points
   separate so each page can gain its own interaction without changing button
   decoding or view navigation. */
static void calendar_key_single_click(void) {}

static void calendar_key_double_click(void) {}

static void calendar_key_long_press(void) {}

static void wifi_setup_key_single_click(void) {}

static void wifi_setup_key_double_click(void) {}

static void wifi_setup_key_long_press(void) { xEventGroupSetBits(ConfigGroups, CFG_REQ_ENABLE); }

void KEY_LoopTask(void* arg) {
  for (;;) {
    EventBits_t even = xEventGroupWaitBits(GP18ButtonGroups, (0x01 | 0x02 | 0x04), pdTRUE, pdFALSE,
                                           pdMS_TO_TICKS(2000));
    if (is_CfgViewOn) {
      if (even & 0x01) {
        wifi_setup_key_single_click();
      } else if (even & 0x02) {
        wifi_setup_key_double_click();
      } else if (even & 0x04) {
        wifi_setup_key_long_press();
      }
    } else {
      if (even & 0x01) {
        calendar_key_single_click();
      } else if (even & 0x02) {
        calendar_key_double_click();
      } else if (even & 0x04) {
        calendar_key_long_press();
      }
    }
  }
}

static void cfg_set_labels(const char *state, const char *ap_hint) {
  static char shown_state[80] = "";
  static char shown_ap_hint[120] = "";
  bool changed = false;

  if (state && strcmp(state, shown_state) != 0) {
    strncpy(shown_state, state, sizeof(shown_state) - 1);
    shown_state[sizeof(shown_state) - 1] = '\0';
    changed = true;
  }
  if (ap_hint && strcmp(ap_hint, shown_ap_hint) != 0) {
    strncpy(shown_ap_hint, ap_hint, sizeof(shown_ap_hint) - 1);
    shown_ap_hint[sizeof(shown_ap_hint) - 1] = '\0';
    changed = true;
  }
  if (!changed) {
    return;
  }
    if(Lvgl_lock(-1)) {
        if(state) {
            lv_label_set_text(init_ui.screen_label_cfg_state, state);
        }
        if(ap_hint) {
            lv_label_set_text(init_ui.screen_label_cfg_ap, ap_hint);
        }
        Lvgl_unlock();
        Lvgl_RequestRender(10);
    }
    ESP_LOGI("cfg", "%s", state ? state : "");
}

void Config_LoopTask(void *arg) {
    bool active = false;
    bool view_open = false;
    for(;;) {
      EventBits_t even =
          xEventGroupWaitBits(ConfigGroups, (CFG_REQ_START | CFG_REQ_STOP | CFG_REQ_ENABLE), pdTRUE,
                              pdFALSE, pdMS_TO_TICKS(500));
      if (even & CFG_REQ_START) {
        view_open = true;
        cfg_set_labels("Long-press KEY\nto enable hotspot",
                       "Hotspot disabled\nLong-press BOOT to exit");
        }
        if(even & CFG_REQ_STOP) {
          view_open = false;
          bool was_active = active;
          if (active) {
            active = false;
            espwifi_config_stop();
            xSemaphoreGive(WifiMutex);
          }
            cfg_set_labels("Long-press BOOT\nto configure", NULL);
            /* Credentials may have just been saved; take the time straight away
               rather than leaving the clock uncorrected until the daily window. */
            if (was_active) {
              xEventGroupSetBits(ConfigGroups, CFG_SYNC_NOW);
            }
            continue;
        }
        if ((even & CFG_REQ_ENABLE) && view_open && !active) {
          active = true;
          /* The label goes up before the mutex: a sync window in progress
             holds the radio for up to half a minute. */
          cfg_set_labels("Starting hotspot...",
                         "Hotspot: " ESPWIFI_AP_SSID "\nPassword: " ESPWIFI_AP_PASS);
          xSemaphoreTake(WifiMutex, portMAX_DELAY);
          if (!is_CfgViewOn) {
            active = false;
            xSemaphoreGive(WifiMutex);
            continue;
          }
          espwifi_config_start();
          cfg_set_labels("Hotspot ready\nJoin it, page pops up",
                         "Hotspot: " ESPWIFI_AP_SSID "\nPassword: " ESPWIFI_AP_PASS
                         "\nPortal: 192.168.4.1");
        }
        if (!active) {
          continue;
        }
        EventBits_t wifi_even = xEventGroupGetBits(wifi_even_);
        if(wifi_even & WIFI_EV_AP_CLIENT) {
            xEventGroupClearBits(wifi_even_,WIFI_EV_AP_CLIENT);
            cfg_set_labels("Phone connected\nOpen the setup page", NULL);
        }
        if(wifi_even & WIFI_EV_CREDENTIALS) {
            xEventGroupClearBits(wifi_even_,WIFI_EV_CREDENTIALS);
            char msg[80];
            snprintf(msg,sizeof(msg),"Connecting to\n%s...",espwifi_cfg_ssid);
            cfg_set_labels(msg, "Portal: 192.168.4.1");
            if(espwifi_config_connect(espwifi_cfg_ssid,espwifi_cfg_pass)) {
                char ip[80];
                snprintf(ip,sizeof(ip),"Connected!\nIP %s",user_esp_bsp._ip);
                cfg_set_labels(ip, "Settings saved\nLong-press BOOT to exit");
            } else {
                cfg_set_labels("Connect failed\nCheck password, retry", "Hotspot: " ESPWIFI_AP_SSID "\nPassword: " ESPWIFI_AP_PASS "\nPortal: 192.168.4.1");
            }
        }
    }
}

void UserApp_AppInit() {
    Custom_ButtonInit();
    Adc_PortInit();
    time_manager_init(&I2cbus);
    sensor_manager_init(&I2cbus);
    ConfigGroups = xEventGroupCreate();
    WifiMutex = xSemaphoreCreateMutex();
    assert(WifiMutex != NULL);
}

void UserApp_UiInit() {
    calendar_ui_data_t empty = {};
    wifi_setup_page_init(&init_ui);
    calendar_ui_create(init_ui.screen);
    calendar_ui_refresh_all(&empty);
#if LVGL_DEBUG_LOG
    calendar_ui_update_uptime((uint32_t)(esp_timer_get_time() / (60LL * 1000000LL)));
#endif
    lv_label_set_text(init_ui.screen_label_cfg_state, "Long-press KEY\nto enable hotspot");
    lv_label_set_text(init_ui.screen_label_cfg_ap, "Hotspot disabled\nLong-press BOOT to exit");
}

void UserApp_TaskInit() {
    xTaskCreatePinnedToCore(Calendar_LoopTask, "Calendar_LoopTask", 5 * 1024, NULL, 2, NULL,1);
    xTaskCreatePinnedToCore(Sensor_LoopTask, "Sensor_LoopTask", 4 * 1024, NULL, 2, NULL,1);
    xTaskCreatePinnedToCore(Battery_LoopTask, "Battery_LoopTask", 4 * 1024, NULL, 2, NULL,1);
    xTaskCreatePinnedToCore(Time_SyncTask, "Time_SyncTask", 5 * 1024, NULL, 2, NULL,1);
    xTaskCreatePinnedToCore(BOOT_LoopTask, "BOOT_LoopTask", 4 * 1024, NULL, 2, NULL,1);
    xTaskCreatePinnedToCore(KEY_LoopTask, "KEY_LoopTask", 4 * 1024, NULL, 2, NULL,1);
    xTaskCreatePinnedToCore(Config_LoopTask, "Config_LoopTask", 5 * 1024, NULL, 2, NULL,1);
}
