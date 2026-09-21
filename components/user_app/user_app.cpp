#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include "button_bsp.h"
#include "user_app.h"
#include "lvgl_bsp.h"
#include "gui_guider.h"
#include "i2c_equipment.h"
#include "i2c_bsp.h"
#include "sdcard_bsp.h"
#include "adc_bsp.h"
#include "esp_wifi_bsp.h"
#include "ble_scan_bsp.h"

static lv_ui init_ui;
I2cMasterBus I2cbus(14,13,0);
CustomSDPort *sdcardPort = NULL;
Shtc3Port *shtc3port = NULL;
EventGroupHandle_t ConfigGroups;

/* ConfigGroups bits */
#define CFG_REQ_START 0x01
#define CFG_REQ_STOP  0x02

static bool is_CfgViewOn = false;

void Lvgl_Cont1Task(void *arg) {
    lv_obj_clear_flag(init_ui.screen_label_1,LV_OBJ_FLAG_HIDDEN); 
    lv_obj_add_flag(init_ui.screen_label_2, LV_OBJ_FLAG_HIDDEN);
    vTaskDelay(pdMS_TO_TICKS(1500));
    lv_obj_clear_flag(init_ui.screen_label_2,LV_OBJ_FLAG_HIDDEN); 
    lv_obj_add_flag(init_ui.screen_label_1, LV_OBJ_FLAG_HIDDEN);
    vTaskDelay(pdMS_TO_TICKS(1500));
    lv_obj_clear_flag(init_ui.screen_cont_2,LV_OBJ_FLAG_HIDDEN); 
    lv_obj_add_flag(init_ui.screen_cont_1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(init_ui.screen_cont_3, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(init_ui.screen_cont_4, LV_OBJ_FLAG_HIDDEN);
    vTaskDelete(NULL); 
}

void Lvgl_UserTask(void *arg) {
    uint32_t times = 0;
    uint32_t adc_time = 0;
    uint32_t rtc_time = 0;
    uint32_t shtc3_time = 0;
    char lvgl_buffer[30] = {""};
    for(;;) {
        if(times - adc_time == 10) {
            adc_time = times;
            uint8_t level = Adc_GetBatteryLevel();
            snprintf(lvgl_buffer,30,"%d%%",level);
            lv_label_set_text(init_ui.screen_label_7, lvgl_buffer);
        }
        if(times - rtc_time == 5) {
            rtc_time = times;
            rtcTimeStruct_t timerData;
            Rtc_GetTime(&timerData);
            snprintf(lvgl_buffer,30,"%02d",timerData.minute);
            lv_label_set_text(init_ui.screen_label_3, lvgl_buffer);
            snprintf(lvgl_buffer,30,"%02d",timerData.second);
            lv_label_set_text(init_ui.screen_label_4, lvgl_buffer);
        }
        if(times - shtc3_time == 25)
        {
            shtc3_time = times;
            float rh,temp;
            shtc3port->Shtc3_ReadTempHumi(&temp,&rh);
            snprintf(lvgl_buffer,30,"%d%%",(int)rh);
            lv_label_set_text(init_ui.screen_label_11, lvgl_buffer);
            snprintf(lvgl_buffer,30,"%d°",(int)temp);
            lv_label_set_text(init_ui.screen_label_12, lvgl_buffer);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
        times++;
    }
}

void Lvgl_SDcardTask(void *arg) {
    const char *str_write = "waveshare.com";
    char str_read[20] = {""};
    if(0 == sdcardPort->SDPort_GetStatus()) {
        lv_label_set_text(init_ui.screen_label_6, "No Card");
    } else {
        sdcardPort->SDPort_WriteFile("/sdcard/sdcard.txt",str_write,strlen(str_write));
        sdcardPort->SDPort_ReadFile("/sdcard/sdcard.txt",(uint8_t *)str_read,NULL);
        if(!strcmp(str_write,str_read)) {
            lv_label_set_text(init_ui.screen_label_6, "passed");
        } else {
            lv_label_set_text(init_ui.screen_label_6, "failed");
        }
    }
    vTaskDelete(NULL);
}

void Lvgl_BleScanTask(void *srg) {
    char send_lvgl[10] = {""};
    uint8_t ble_scan_count = 0;
    uint8_t ble_mac[6];
    /* If credentials are stored, give the STA connection time to come up and
       leave the radio to Wi-Fi: BLE and Wi-Fi are not coexistent here. */
    for(int i = 0; i < 40 && espwifi_is_active() && !user_esp_bsp._ip[0]; i++) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    if(espwifi_is_active()) {
        if(Lvgl_lock(-1)) {
            lv_label_set_text(init_ui.screen_label_14, user_esp_bsp._ip[0] ? user_esp_bsp._ip : "OFFLINE");
            lv_label_set_text(init_ui.screen_label_13, "-");
            Lvgl_unlock();
        }
        vTaskDelete(NULL);
    }
    ble_scan_prepare();
    ble_stack_init();
    ble_scan_start();
    for(;xQueueReceive(ble_queue,ble_mac,3500) == pdTRUE;) {
        ble_scan_count++;
        if(ble_scan_count >= 20)
        break;
        vTaskDelay(pdMS_TO_TICKS(30));
    }
    snprintf(send_lvgl,10,"%d",ble_scan_count);
    if(Lvgl_lock(-1)) {
        lv_label_set_text(init_ui.screen_label_14, "SETUP");
        lv_label_set_text(init_ui.screen_label_13, send_lvgl);
        Lvgl_unlock();
    }
    ble_stack_deinit();    //释放BLE
    vTaskDelete(NULL);
}

void BOOT_LoopTask(void *arg) {
    for(;;) {
        EventBits_t even = xEventGroupWaitBits(BootButtonGroups,(0x01 | 0x02 | 0x04),pdTRUE,pdFALSE,pdMS_TO_TICKS(2000));
        if(even & 0x04) {
            if(0 == is_CfgViewOn) {
                is_CfgViewOn = 1;
                if(Lvgl_lock(-1)) {
                    lv_obj_clear_flag(init_ui.screen_cont_4,LV_OBJ_FLAG_HIDDEN); 
                    lv_obj_add_flag(init_ui.screen_cont_1, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(init_ui.screen_cont_2, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(init_ui.screen_cont_3, LV_OBJ_FLAG_HIDDEN);
                    Lvgl_unlock();
                }
                xEventGroupSetBits(ConfigGroups,CFG_REQ_START);
            } else {
                is_CfgViewOn = 0;
                if(Lvgl_lock(-1)) {
                    lv_obj_clear_flag(init_ui.screen_cont_2,LV_OBJ_FLAG_HIDDEN); 
                    lv_obj_add_flag(init_ui.screen_cont_1, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(init_ui.screen_cont_4, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(init_ui.screen_cont_3, LV_OBJ_FLAG_HIDDEN);
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
                    lv_obj_clear_flag(init_ui.screen_cont_3,LV_OBJ_FLAG_HIDDEN); 
                    lv_obj_add_flag(init_ui.screen_cont_1, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(init_ui.screen_cont_2, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(init_ui.screen_cont_4, LV_OBJ_FLAG_HIDDEN);
                    Lvgl_unlock();
                }
            } else {
                is_cont3en = 0;
                if(Lvgl_lock(-1)) {
                    lv_obj_clear_flag(init_ui.screen_cont_2,LV_OBJ_FLAG_HIDDEN); 
                    lv_obj_add_flag(init_ui.screen_cont_1, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(init_ui.screen_cont_3, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_add_flag(init_ui.screen_cont_4, LV_OBJ_FLAG_HIDDEN);
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
                if(Lvgl_lock(-1)) {
                    lv_label_set_text(init_ui.screen_label_14, user_esp_bsp._ip);
                    Lvgl_unlock();
                }
            } else {
                cfg_set_labels("Connect failed\nCheck password, retry", "Hotspot: " ESPWIFI_AP_SSID "\nPassword: " ESPWIFI_AP_PASS "\nPortal: 192.168.4.1");
            }
        }
    }
}

void UserApp_AppInit() {
    sdcardPort = new CustomSDPort("/sdcard");
    Adc_PortInit();
    Custom_ButtonInit();
    Rtc_Setup(&I2cbus,0x51);
    Rtc_SetTime(2026,1,5,14,30,30);
    shtc3port = new Shtc3Port(I2cbus);
    ConfigGroups = xEventGroupCreate();
    espwifi_connect_stored();
}

void UserApp_UiInit() {
    setup_ui(&init_ui);
    lv_label_set_text(init_ui.screen_label_8, "ON");
    lv_label_set_text(init_ui.screen_label_cfg_state, "Long-press BOOT\nto configure");
}

void UserApp_TaskInit() {
    xTaskCreatePinnedToCore(Lvgl_Cont1Task, "Lvgl_Cont1Task", 4 * 1024, NULL, 2, NULL,1);
    xTaskCreatePinnedToCore(Lvgl_UserTask, "Lvgl_UserTask", 5 * 1024, NULL, 2, NULL,1);
    xTaskCreatePinnedToCore(Lvgl_SDcardTask, "Lvgl_SDcardTask", 4 * 1024, NULL, 2, NULL,1);
    xTaskCreatePinnedToCore(Lvgl_BleScanTask, "Lvgl_BleScanTask", 4 * 1024, NULL, 2, NULL,1);
    xTaskCreatePinnedToCore(BOOT_LoopTask, "BOOT_LoopTask", 4 * 1024, NULL, 2, NULL,1);
    xTaskCreatePinnedToCore(KEY_LoopTask, "KEY_LoopTask", 4 * 1024, NULL, 2, NULL,1);
    xTaskCreatePinnedToCore(Config_LoopTask, "Config_LoopTask", 5 * 1024, NULL, 2, NULL,1);
}
