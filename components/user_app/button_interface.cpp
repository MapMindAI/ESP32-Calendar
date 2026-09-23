#include "user_app_internal.h"

#include <esp_log.h>
#include <stdio.h>
#include <string.h>
#include "button_bsp.h"
#include "calendar_ui.h"
#include "esp_wifi_bsp.h"
#include "lvgl_bsp.h"
#include "tarot_manager.h"
#include "tarot_page.h"

static void show_view(lv_obj_t* view) {
  lv_obj_add_flag(calendar_ui_root(), LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(WifiSetupPage.screen_cont_wifi_setup, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(tarot_page_root(), LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(view, LV_OBJ_FLAG_HIDDEN);
}

void BOOT_LoopTask(void* arg) {
  for (;;) {
    EventBits_t event = xEventGroupWaitBits(BootButtonGroups, 0x01 | 0x02 | 0x04, pdTRUE, pdFALSE,
                                            pdMS_TO_TICKS(2000));
    if (!(event & 0x04)) {
      continue;
    }

    app_view_t previous = CurrentView;
    switch (CurrentView) {
      case APP_VIEW_CALENDAR:
        CurrentView = APP_VIEW_TAROT;
        break;
      case APP_VIEW_TAROT:
        CurrentView = APP_VIEW_WIFI;
        break;
      default:
        CurrentView = APP_VIEW_CALENDAR;
        break;
    }
    /* Leaving the setup view always stops the hotspot/portal if it is running. */
    if (previous == APP_VIEW_WIFI) {
      xEventGroupSetBits(ConfigGroups, CFG_REQ_STOP);
    }

    lv_obj_t* view = calendar_ui_root();
    if (CurrentView == APP_VIEW_TAROT) {
      view = tarot_page_root();
    } else if (CurrentView == APP_VIEW_WIFI) {
      view = WifiSetupPage.screen_cont_wifi_setup;
    }
    if (Lvgl_lock(-1)) {
      show_view(view);
      Lvgl_unlock();
      Lvgl_RequestRender(6);
    }

    if (CurrentView == APP_VIEW_WIFI) {
      xEventGroupSetBits(ConfigGroups, CFG_REQ_START);
    } else if (CurrentView == APP_VIEW_TAROT) {
      Tarot_ShowRandom();
    }
  }
}

static void calendar_key_single_click(void) {
  CalendarView_AdvanceDay();
  CalendarView_Render();
}
static void calendar_key_double_click(void) {
  CalendarView_AdvanceMonth();
  CalendarView_Render();
}
static void calendar_key_long_press(void) {
  CalendarView_ResetDay();
  CalendarView_Render();
}
static void tarot_key_single_click(void) { Tarot_ShowRandom(); }
static void tarot_key_double_click(void) {}
static void tarot_key_long_press(void) {}
static void wifi_setup_key_single_click(void) {}
static void wifi_setup_key_double_click(void) {}

static void wifi_setup_key_long_press(void) { xEventGroupSetBits(ConfigGroups, CFG_REQ_ENABLE); }

void KEY_LoopTask(void* arg) {
  for (;;) {
    EventBits_t event = xEventGroupWaitBits(GP18ButtonGroups, 0x01 | 0x02 | 0x04, pdTRUE, pdFALSE,
                                            pdMS_TO_TICKS(2000));
    switch (CurrentView) {
      case APP_VIEW_WIFI:
        if (event & 0x01) {
          wifi_setup_key_single_click();
        } else if (event & 0x02) {
          wifi_setup_key_double_click();
        } else if (event & 0x04) {
          wifi_setup_key_long_press();
        }
        break;
      case APP_VIEW_TAROT:
        if (event & 0x01) {
          tarot_key_single_click();
        } else if (event & 0x02) {
          tarot_key_double_click();
        } else if (event & 0x04) {
          tarot_key_long_press();
        }
        break;
      default:
        if (event & 0x01) {
          calendar_key_single_click();
        } else if (event & 0x02) {
          calendar_key_double_click();
        } else if (event & 0x04) {
          calendar_key_long_press();
        }
        break;
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
      if (CurrentView != APP_VIEW_WIFI) {
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
