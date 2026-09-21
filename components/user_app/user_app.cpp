#include <stdio.h>
#include <math.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include "button_bsp.h"
#include "user_app.h"
#include "lvgl_bsp.h"
#include "gui_guider.h"
#include "calendar_ui.h"
#include "calendar_calc.h"
#include "i2c_bsp.h"
#include "time_manager.h"
#include "sensor_manager.h"
#include "esp_wifi_bsp.h"

static lv_ui init_ui;
I2cMasterBus I2cbus(14,13,0);
EventGroupHandle_t ConfigGroups;

/* ConfigGroups bits */
#define CFG_REQ_START 0x01
#define CFG_REQ_STOP  0x02

/* The panel repaints in full on any invalidation, so a widget is only written
   when the value behind it actually moved: the clock at minute resolution, the
   environment row on a threshold, everything else when the date rolls over. */
#define SENSOR_READ_INTERVAL_MS      60000
#define TEMPERATURE_REFRESH_THRESHOLD 0.2f
#define HUMIDITY_REFRESH_THRESHOLD    1.0f
#define ENVIRONMENT_MAX_REFRESH_MIN   10

static bool is_CfgViewOn = false;

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
    for(;;) {
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
            }
            last_minute = local.tm_min;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Temperature and humidity. Reads once a minute; repaints only when the value
   crosses a threshold, or after ENVIRONMENT_MAX_REFRESH_MIN of no movement. */
void Sensor_LoopTask(void *arg) {
    environment_data_t environment;
    float shown_temperature = 0.0f;
    float shown_humidity = 0.0f;
    bool shown_valid = false;
    uint32_t minutes_since_refresh = ENVIRONMENT_MAX_REFRESH_MIN;
    for(;;) {
        sensor_manager_read(&environment);
        bool publish = false;
        if(environment.valid != shown_valid) {
            publish = true;
        } else if(environment.valid &&
                  (fabsf(environment.temperature_c - shown_temperature) >= TEMPERATURE_REFRESH_THRESHOLD ||
                   fabsf(environment.humidity_percent - shown_humidity) >= HUMIDITY_REFRESH_THRESHOLD)) {
            publish = true;
        } else if(minutes_since_refresh >= ENVIRONMENT_MAX_REFRESH_MIN) {
            publish = true;
        }
        if(publish) {
            if(Lvgl_lock(-1)) {
                calendar_ui_update_environment(environment.temperature_c,environment.humidity_percent,environment.valid);
                Lvgl_unlock();
            }
            shown_temperature = environment.temperature_c;
            shown_humidity = environment.humidity_percent;
            shown_valid = environment.valid;
            minutes_since_refresh = 0;
        } else {
            minutes_since_refresh++;
        }
        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
    }
}

/* Waits for the station to get an address — at boot from stored credentials, or
   later from the setup view — then hands the clock over to SNTP and exits. */
void Time_SyncTask(void *arg) {
    for(;!user_esp_bsp._ip[0];) {
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    time_manager_start_sntp();
    vTaskDelete(NULL);
}

static void show_view(lv_obj_t *view)
{
    lv_obj_add_flag(calendar_ui_root(), LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(init_ui.screen_cont_3, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(init_ui.screen_cont_4, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(view, LV_OBJ_FLAG_HIDDEN);
}

void BOOT_LoopTask(void *arg) {
    for(;;) {
        EventBits_t even = xEventGroupWaitBits(BootButtonGroups,(0x01 | 0x02 | 0x04),pdTRUE,pdFALSE,pdMS_TO_TICKS(2000));
        if(even & 0x04) {
            if(0 == is_CfgViewOn) {
                is_CfgViewOn = 1;
                if(Lvgl_lock(-1)) {
                    show_view(init_ui.screen_cont_4);
                    Lvgl_unlock();
                }
                xEventGroupSetBits(ConfigGroups,CFG_REQ_START);
            } else {
                is_CfgViewOn = 0;
                if(Lvgl_lock(-1)) {
                    show_view(calendar_ui_root());
                    Lvgl_unlock();
                }
                xEventGroupSetBits(ConfigGroups,CFG_REQ_STOP);
            }
        }
    }
}

void KEY_LoopTask(void *arg) {
    bool is_cont3en = 0;
    for(;;) {
        EventBits_t even = xEventGroupWaitBits(GP18ButtonGroups,(0x01 | 0x02 | 0x04),pdTRUE,pdFALSE,pdMS_TO_TICKS(2000));
        if(even & 0x04) {
            if(0 == is_cont3en) {
                is_cont3en = 1;
                if(Lvgl_lock(-1)) {
                    show_view(init_ui.screen_cont_3);
                    Lvgl_unlock();
                }
            } else {
                is_cont3en = 0;
                if(Lvgl_lock(-1)) {
                    show_view(calendar_ui_root());
                    Lvgl_unlock();
                }
            }
        }
    }
}

static void cfg_set_labels(const char *state, const char *ap_hint) {
    if(Lvgl_lock(-1)) {
        if(state) {
            lv_label_set_text(init_ui.screen_label_cfg_state, state);
        }
        if(ap_hint) {
            lv_label_set_text(init_ui.screen_label_cfg_ap, ap_hint);
        }
        Lvgl_unlock();
    }
    ESP_LOGI("cfg", "%s", state ? state : "");
}

void Config_LoopTask(void *arg) {
    bool active = false;
    for(;;) {
        EventBits_t even = xEventGroupWaitBits(ConfigGroups,(CFG_REQ_START | CFG_REQ_STOP),pdTRUE,pdFALSE,pdMS_TO_TICKS(500));
        if(even & CFG_REQ_START) {
            if(!active) {
                active = true;
                cfg_set_labels("Starting hotspot...", "Hotspot: " ESPWIFI_AP_SSID "\nPassword: " ESPWIFI_AP_PASS);
                espwifi_config_start();
                cfg_set_labels("Hotspot ready\nJoin it, page pops up", "Hotspot: " ESPWIFI_AP_SSID "\nPassword: " ESPWIFI_AP_PASS "\nPortal: 192.168.4.1");
            }
        }
        if(!active) {
            continue;
        }
        if(even & CFG_REQ_STOP) {
            active = false;
            espwifi_config_stop();
            cfg_set_labels("Long-press BOOT\nto configure", NULL);
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
    time_manager_init(&I2cbus);
    sensor_manager_init(&I2cbus);
    ConfigGroups = xEventGroupCreate();
    espwifi_connect_stored();
}

void UserApp_UiInit() {
    calendar_ui_data_t empty = {};
    setup_ui(&init_ui);
    /* The GUI Guider dashboard is replaced wholesale. Deleting it here keeps the
       generated screen file untouched and regenerable. */
    lv_obj_del(init_ui.screen_cont_2);
    init_ui.screen_cont_2 = NULL;
    calendar_ui_create(init_ui.screen);
    calendar_ui_refresh_all(&empty);
    lv_label_set_text(init_ui.screen_label_cfg_state, "Long-press BOOT\nto configure");
}

void UserApp_TaskInit() {
    xTaskCreatePinnedToCore(Calendar_LoopTask, "Calendar_LoopTask", 5 * 1024, NULL, 2, NULL,1);
    xTaskCreatePinnedToCore(Sensor_LoopTask, "Sensor_LoopTask", 4 * 1024, NULL, 2, NULL,1);
    xTaskCreatePinnedToCore(Time_SyncTask, "Time_SyncTask", 4 * 1024, NULL, 2, NULL,1);
    xTaskCreatePinnedToCore(BOOT_LoopTask, "BOOT_LoopTask", 4 * 1024, NULL, 2, NULL,1);
    xTaskCreatePinnedToCore(KEY_LoopTask, "KEY_LoopTask", 4 * 1024, NULL, 2, NULL,1);
    xTaskCreatePinnedToCore(Config_LoopTask, "Config_LoopTask", 5 * 1024, NULL, 2, NULL,1);
}
