#include "user_app_internal.h"

#include <esp_log.h>
#include <stdio.h>
#include <string.h>
#include "button_bsp.h"
#include "calendar_ui.h"
#include "esp_wifi_bsp.h"
#include "lvgl_bsp.h"

static void show_view(lv_obj_t* view) {
  lv_obj_add_flag(calendar_ui_root(), LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(WifiSetupPage.screen_cont_wifi_setup, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(view, LV_OBJ_FLAG_HIDDEN);
}

void BOOT_LoopTask(void* arg) {
  for (;;) {
    EventBits_t event = xEventGroupWaitBits(BootButtonGroups, 0x01 | 0x02 | 0x04, pdTRUE, pdFALSE,
                                            pdMS_TO_TICKS(2000));
    if (!(event & 0x04)) {
      continue;
    }
    if (!IsCfgViewOn) {
      IsCfgViewOn = true;
      if (Lvgl_lock(-1)) {
        show_view(WifiSetupPage.screen_cont_wifi_setup);
        Lvgl_unlock();
        Lvgl_RequestRender(6);
      }
      xEventGroupSetBits(ConfigGroups, CFG_REQ_START);
    } else {
      IsCfgViewOn = false;
      if (Lvgl_lock(-1)) {
        show_view(calendar_ui_root());
        Lvgl_unlock();
        Lvgl_RequestRender(7);
      }
      xEventGroupSetBits(ConfigGroups, CFG_REQ_STOP);
    }
  }
}

static void calendar_key_single_click(void) {}
static void calendar_key_double_click(void) {}
static void calendar_key_long_press(void) {}
static void wifi_setup_key_single_click(void) {}
static void wifi_setup_key_double_click(void) {}

static void wifi_setup_key_long_press(void) { xEventGroupSetBits(ConfigGroups, CFG_REQ_ENABLE); }

void KEY_LoopTask(void* arg) {
  for (;;) {
    EventBits_t event = xEventGroupWaitBits(GP18ButtonGroups, 0x01 | 0x02 | 0x04, pdTRUE, pdFALSE,
                                            pdMS_TO_TICKS(2000));
    if (IsCfgViewOn) {
      if (event & 0x01) {
        wifi_setup_key_single_click();
      } else if (event & 0x02) {
        wifi_setup_key_double_click();
      } else if (event & 0x04) {
        wifi_setup_key_long_press();
      }
    } else if (event & 0x01) {
      calendar_key_single_click();
    } else if (event & 0x02) {
      calendar_key_double_click();
    } else if (event & 0x04) {
      calendar_key_long_press();
    }
  }
}

static void config_set_labels(const char* state, const char* ap_hint) {
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
  if (Lvgl_lock(-1)) {
    if (state) {
      lv_label_set_text(WifiSetupPage.screen_label_cfg_state, state);
    }
    if (ap_hint) {
      lv_label_set_text(WifiSetupPage.screen_label_cfg_ap, ap_hint);
    }
    Lvgl_unlock();
    Lvgl_RequestRender(10);
  }
  ESP_LOGI("cfg", "%s", state ? state : "");
}

void Config_LoopTask(void* arg) {
  bool active = false;
  bool view_open = false;

  for (;;) {
    EventBits_t event =
        xEventGroupWaitBits(ConfigGroups, CFG_REQ_START | CFG_REQ_STOP | CFG_REQ_ENABLE, pdTRUE,
                            pdFALSE, pdMS_TO_TICKS(500));
    if (event & CFG_REQ_START) {
      view_open = true;
      config_set_labels("Long-press KEY\nto enable hotspot",
                        "Hotspot disabled\nLong-press BOOT to exit");
    }
    if (event & CFG_REQ_STOP) {
      view_open = false;
      bool was_active = active;
      if (active) {
        active = false;
        espwifi_config_stop();
        xSemaphoreGive(WifiMutex);
      }
      config_set_labels("Long-press BOOT\nto configure", NULL);
      if (was_active) {
        xEventGroupSetBits(ConfigGroups, CFG_SYNC_NOW);
      }
      continue;
    }
    if ((event & CFG_REQ_ENABLE) && view_open && !active) {
      active = true;
      config_set_labels("Starting hotspot...",
                        "Hotspot: " ESPWIFI_AP_SSID "\nPassword: " ESPWIFI_AP_PASS);
      xSemaphoreTake(WifiMutex, portMAX_DELAY);
      if (!IsCfgViewOn) {
        active = false;
        xSemaphoreGive(WifiMutex);
        continue;
      }
      espwifi_config_start();
      config_set_labels("Hotspot ready\nJoin it, page pops up",
                        "Hotspot: " ESPWIFI_AP_SSID "\nPassword: " ESPWIFI_AP_PASS
                        "\nPortal: 192.168.4.1");
    }
    if (!active) {
      continue;
    }
    EventBits_t wifi_event = xEventGroupGetBits(wifi_even_);
    if (wifi_event & WIFI_EV_AP_CLIENT) {
      xEventGroupClearBits(wifi_even_, WIFI_EV_AP_CLIENT);
      config_set_labels("Phone connected\nOpen the setup page", NULL);
    }
    if (wifi_event & WIFI_EV_CREDENTIALS) {
      xEventGroupClearBits(wifi_even_, WIFI_EV_CREDENTIALS);
      char message[80];
      snprintf(message, sizeof(message), "Connecting to\n%s...", espwifi_cfg_ssid);
      config_set_labels(message, "Portal: 192.168.4.1");
      if (espwifi_config_connect(espwifi_cfg_ssid, espwifi_cfg_pass)) {
        char ip_address[80];
        snprintf(ip_address, sizeof(ip_address), "Connected!\nIP %s", user_esp_bsp._ip);
        config_set_labels(ip_address, "Settings saved\nLong-press BOOT to exit");
      } else {
        config_set_labels("Connect failed\nCheck password, retry",
                          "Hotspot: " ESPWIFI_AP_SSID "\nPassword: " ESPWIFI_AP_PASS
                          "\nPortal: 192.168.4.1");
      }
    }
  }
}
