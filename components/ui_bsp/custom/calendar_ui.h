#ifndef CALENDAR_UI_H
#define CALENDAR_UI_H

#include <stdint.h>

#include "lvgl.h"
#include "calendar_calc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The status dashboard: clock, date, month calendar and the environment row.
 *
 * ui_bsp sits below app_bsp, so none of these take the LVGL lock themselves —
 * every call must be made from the LVGL task or between Lvgl_lock()/Lvgl_unlock().
 */

/* Build the dashboard as a full-screen container on `parent`. */
void calendar_ui_create(lv_obj_t *parent);

/* The container, for the show/hide juggling the view switcher does. */
lv_obj_t *calendar_ui_root(void);

/* Redraw everything. Used at boot and whenever the date rolls over. */
void calendar_ui_refresh_all(const calendar_ui_data_t *data);

/* Minute cadence: only the big clock. */
void calendar_ui_update_time(int hour, int minute, bool valid);

/* Sensor cadence: only the temperature/humidity row. */
void calendar_ui_update_environment(float temperature_c, float humidity_percent, bool valid);

/* Battery cadence: only the lower-left status label. */
void calendar_ui_update_battery(uint8_t percent);

#ifdef __cplusplus
}
#endif

#endif
