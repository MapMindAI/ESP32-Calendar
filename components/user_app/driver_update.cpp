#include "user_app_internal.h"

#include <esp_log.h>
#include <esp_timer.h>
#include <math.h>
#include "adc_bsp.h"
#include "calendar_calc.h"
#include "calendar_ui.h"
#include "esp_wifi_bsp.h"
#include "lvgl_bsp.h"
#include "sensor_manager.h"
#include "time_manager.h"

#define TIME_SYNC_HOUR 3
#define TIME_SYNC_MINUTE 30
#define TIME_SYNC_IP_TIMEOUT_MS (20 * 1000)
#define TIME_SYNC_SNTP_TIMEOUT_MS (15 * 1000)
#define TIME_SYNC_RETRY_MINUTES 30

#define SENSOR_READ_INTERVAL_MS 60000
#define TEMPERATURE_REFRESH_THRESHOLD 0.2f
#define HUMIDITY_REFRESH_THRESHOLD 1.0f
#define BATTERY_READ_INTERVAL_MS 60000

/* Browsing offsets the KEY button has walked the calendar view away from
   today: whole months first, then single days on top. Only ever incremented
   and reset, so the month math never goes below today's month. Written from
   KEY_LoopTask, read from Calendar_LoopTask; plain volatile ints and the flag
   are atomic enough for counters. */
static volatile int calendar_view_offset_months = 0;
static volatile int calendar_view_offset_days = 0;
/* Set by a month step: anchor the view on the 1st instead of today's day. */
static volatile bool calendar_view_anchor_first = false;

static void calendar_view_render(void);

/* Today + the browsing offsets, anchored on the 1st after a month step and
   otherwise on today, with the day clamped into the target month (today is
   the 31st and the target month has 30 days -> the 30th). */
static bool calendar_view_date(struct tm* view) {
  struct tm local;
  int month_index;
  int year;
  int month;
  int days_in_month;

  if (!time_manager_get_local(&local)) {
    return false;
  }
  month_index = local.tm_mon + calendar_view_offset_months;
  year = local.tm_year + 1900 + month_index / 12;
  month = month_index % 12 + 1;
  days_in_month = calendar_calc_days_in_month(year, month);

  *view = local;
  view->tm_year = year - 1900;
  view->tm_mon = month - 1;
  view->tm_mday = calendar_view_anchor_first ? 1 : view->tm_mday;
  if (view->tm_mday > days_in_month) {
    view->tm_mday = days_in_month;
  }
  view->tm_mday += calendar_view_offset_days;
  mktime(view);
  return true;
}

void CalendarView_AdvanceDay(void) { calendar_view_offset_days = calendar_view_offset_days + 1; }

/* Next month, always on its 1st. */
void CalendarView_AdvanceMonth(void) {
  calendar_view_offset_months = calendar_view_offset_months + 1;
  calendar_view_offset_days = 0;
  calendar_view_anchor_first = true;
}

void CalendarView_ResetDay(void) {
  calendar_view_offset_months = 0;
  calendar_view_offset_days = 0;
  calendar_view_anchor_first = false;
}

/* Button-triggered re-render: log the resulting view date, then repaint. */
void CalendarView_Render(void) {
  calendar_view_render();
}

/* Rebuild the dashboard for today + the browsing offset and push it to the
   panel. Shared by Calendar_LoopTask's day-change refresh and the KEY button
   handlers. */
static void calendar_view_render(void) {
  environment_data_t environment;
  struct tm local;
  struct tm view;
  calendar_ui_data_t data = {};

  if (!calendar_view_date(&view)) {
    return;
  }
  ESP_LOGI("cal_view", "offset %+dm %+dd -> %04d-%02d-%02d", calendar_view_offset_months,
           calendar_view_offset_days, view.tm_year + 1900, view.tm_mon + 1, view.tm_mday);
  time_manager_get_local(&local);

  calendar_calc_fill(&data, &view);
  data.browsing = calendar_view_offset_months != 0 || calendar_view_offset_days != 0;
  data.today_year = local.tm_year + 1900;
  data.today_month = local.tm_mon + 1;
  data.today_day = local.tm_mday;

  sensor_manager_last(&environment);
  data.temperature_c = environment.temperature_c;
  data.humidity_percent = environment.humidity_percent;
  data.environment_valid = environment.valid;

  if (Lvgl_lock(-1)) {
    calendar_ui_refresh_all(&data);
    Lvgl_unlock();
    Lvgl_RequestRender(1);
  }
}

void Calendar_LoopTask(void* arg) {
  struct tm local;
  int last_year = -1;
  int last_month = -1;
  int last_minute = -1;
  int last_day = -1;
  bool last_valid = false;
#if LVGL_DEBUG_LOG
  uint32_t last_uptime_minutes = UINT32_MAX;
#endif

  for (;;) {
#if LVGL_DEBUG_LOG
    uint32_t uptime_minutes = (uint32_t)(esp_timer_get_time() / (60LL * 1000000LL));
    if (uptime_minutes != last_uptime_minutes && Lvgl_lock(-1)) {
      calendar_ui_update_uptime(uptime_minutes);
      Lvgl_unlock();
      Lvgl_RequestRender(11);
      last_uptime_minutes = uptime_minutes;
    }
#endif
    bool valid = time_manager_get_local(&local);
    if (!valid) {
      last_valid = false;
      last_year = -1;
      last_month = -1;
      last_minute = -1;
      last_day = -1;
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }
    if (!last_valid || local.tm_year != last_year || local.tm_mon != last_month ||
        local.tm_mday != last_day) {
      calendar_view_render();
      last_valid = true;
      last_year = local.tm_year;
      last_month = local.tm_mon;
      last_day = local.tm_mday;
      last_minute = local.tm_min;
    } else if (local.tm_min != last_minute) {
      if (Lvgl_lock(-1)) {
        calendar_ui_update_time(local.tm_hour, local.tm_min, true);
        Lvgl_unlock();
        Lvgl_RequestRender(2);
      }
      last_minute = local.tm_min;
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void Battery_LoopTask(void* arg) {
  int shown_level = -1;

  for (;;) {
    uint8_t level = Adc_GetBatteryLevel();
    if (level != shown_level && Lvgl_lock(-1)) {
      calendar_ui_update_battery(level);
      Lvgl_unlock();
      Lvgl_RequestRender(3);
      shown_level = level;
    }
    vTaskDelay(pdMS_TO_TICKS(BATTERY_READ_INTERVAL_MS));
  }
}

void Sensor_LoopTask(void* arg) {
  environment_data_t environment;
  float shown_temperature = 0.0f;
  float shown_humidity = 0.0f;
  bool shown_valid = false;

  for (;;) {
    sensor_manager_read(&environment);
    bool publish =
        environment.valid != shown_valid ||
        (environment.valid &&
         (fabsf(environment.temperature_c - shown_temperature) >= TEMPERATURE_REFRESH_THRESHOLD ||
          fabsf(environment.humidity_percent - shown_humidity) >= HUMIDITY_REFRESH_THRESHOLD));
    if (publish && Lvgl_lock(-1)) {
      calendar_ui_update_environment(environment.temperature_c, environment.humidity_percent,
                                     environment.valid);
      Lvgl_unlock();
      Lvgl_RequestRender(4);
      shown_temperature = environment.temperature_c;
      shown_humidity = environment.humidity_percent;
      shown_valid = environment.valid;
    }
    vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
  }
}

static void wifi_status_publish(calendar_wifi_state_t state) {
  static calendar_wifi_state_t shown_state = CALENDAR_WIFI_UNSET;

  if (state != shown_state && Lvgl_lock(-1)) {
    calendar_ui_update_wifi(state);
    Lvgl_unlock();
    Lvgl_RequestRender(5);
    shown_state = state;
  }
}

static bool time_sync_window(void) {
  bool synced = false;

  xSemaphoreTake(WifiMutex, portMAX_DELAY);
  wifi_status_publish(CALENDAR_WIFI_ACTIVE);
  if (!espwifi_connect_stored()) {
    wifi_status_publish(CALENDAR_WIFI_UNSET);
    xSemaphoreGive(WifiMutex);
    return false;
  }
  if (espwifi_wait_for_ip(TIME_SYNC_IP_TIMEOUT_MS)) {
    synced = time_manager_sync_now(TIME_SYNC_SNTP_TIMEOUT_MS);
  }
  espwifi_deinit();
  wifi_status_publish(synced ? CALENDAR_WIFI_SYNCED : CALENDAR_WIFI_FAILED);
  xSemaphoreGive(WifiMutex);
  return synced;
}

static uint32_t seconds_until_next_window(bool synced) {
  struct tm local;

  if (!synced || !time_manager_get_local(&local)) {
    return TIME_SYNC_RETRY_MINUTES * 60;
  }
  int now_second = local.tm_hour * 3600 + local.tm_min * 60 + local.tm_sec;
  int next_second = TIME_SYNC_HOUR * 3600 + TIME_SYNC_MINUTE * 60;
  int remaining = next_second - now_second;

  if (remaining <= 0) {
    remaining += 24 * 3600;
  }
  return (uint32_t)remaining;
}

void Time_SyncTask(void* arg) {
  for (;;) {
    bool synced = time_sync_window();
    uint32_t wait_s = seconds_until_next_window(synced);

    xEventGroupWaitBits(ConfigGroups, CFG_SYNC_NOW, pdTRUE, pdFALSE, pdMS_TO_TICKS(wait_s * 1000U));
  }
}
