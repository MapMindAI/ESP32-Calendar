#include "user_app.h"
#include "user_app_internal.h"

#include <assert.h>
#include "adc_bsp.h"
#include "button_bsp.h"
#include "calendar_ui.h"
#include "sensor_manager.h"
#include "time_manager.h"

I2cMasterBus I2cbus(14, 13, 0);
EventGroupHandle_t ConfigGroups;
SemaphoreHandle_t WifiMutex;
wifi_setup_page_t WifiSetupPage;
bool IsCfgViewOn = false;

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

  wifi_setup_page_init(&WifiSetupPage);
  calendar_ui_create(WifiSetupPage.screen);
  calendar_ui_refresh_all(&empty);
#if LVGL_DEBUG_LOG
    calendar_ui_update_uptime((uint32_t)(esp_timer_get_time() / (60LL * 1000000LL)));
#endif
    lv_label_set_text(WifiSetupPage.screen_label_cfg_state, "Long-press KEY\nto enable hotspot");
    lv_label_set_text(WifiSetupPage.screen_label_cfg_ap,
                      "Hotspot disabled\nLong-press BOOT to exit");
}

void UserApp_TaskInit() {
  xTaskCreatePinnedToCore(Calendar_LoopTask, "Calendar_LoopTask", 5 * 1024, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(Sensor_LoopTask, "Sensor_LoopTask", 4 * 1024, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(Battery_LoopTask, "Battery_LoopTask", 4 * 1024, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(Time_SyncTask, "Time_SyncTask", 5 * 1024, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(BOOT_LoopTask, "BOOT_LoopTask", 4 * 1024, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(KEY_LoopTask, "KEY_LoopTask", 4 * 1024, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(Config_LoopTask, "Config_LoopTask", 5 * 1024, NULL, 2, NULL, 1);
}
