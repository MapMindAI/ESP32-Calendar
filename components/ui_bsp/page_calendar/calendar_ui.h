#ifndef CALENDAR_UI_H
#define CALENDAR_UI_H

#include <stdint.h>

#include "calendar_calc.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The status dashboard: clock, date, month calendar and the environment row.
 *
 * ui_bsp sits below app_bsp, so none of these take the LVGL lock themselves —
 * every call must be made from the LVGL task or between Lvgl_lock()/Lvgl_unlock().
 */

/* Build the dashboard as a full-screen container on `parent`. */
void calendar_ui_create(lv_obj_t* parent);

/* The container, for the show/hide juggling the view switcher does. */
lv_obj_t* calendar_ui_root(void);

/* Redraw everything. Used at boot and whenever the date rolls over. */
void calendar_ui_refresh_all(const calendar_ui_data_t* data);

/* Minute cadence: only the big clock. */
void calendar_ui_update_time(int hour, int minute, bool valid);

/* Sensor cadence: only the temperature/humidity row. */
void calendar_ui_update_environment(float temperature_c, float humidity_percent, bool valid);

/* Battery cadence: only the lower-right percentage label. */
void calendar_ui_update_battery(uint8_t percent);

/* What the Wi-Fi icon under the battery says. The radio is down between the
   daily sync windows, so the icon reports how the last one went rather than a
   live link state. */
typedef enum {
  CALENDAR_WIFI_UNSET,  /* no credentials stored — the setup view is the fix */
  CALENDAR_WIFI_ACTIVE, /* radio up right now: a sync window or the portal */
  CALENDAR_WIFI_SYNCED, /* radio down again, the last window got the time */
  CALENDAR_WIFI_FAILED, /* radio down, the last window did not */
} calendar_wifi_state_t;

/* Sync cadence: only the lower-right Wi-Fi icon. */
void calendar_ui_update_wifi(calendar_wifi_state_t state);

/* Debug-only lower-right status: elapsed time since the ESP booted. */
void calendar_ui_update_uptime(uint32_t elapsed_minutes);

#ifdef __cplusplus
}
#endif

#endif
