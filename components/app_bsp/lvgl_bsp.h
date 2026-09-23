#pragma once

#include <stdint.h>
#include "lvgl.h"

#define LVGL_TICK_PERIOD_MS    5

/* Set to 1 while diagnosing display updates to log each completed render. */
#ifndef LVGL_DEBUG_LOG
#define LVGL_DEBUG_LOG 1
#endif

typedef void (*DispFlushCb)(struct _lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);

void Lvgl_PortInit(int width, int height, DispFlushCb flush_cb);
bool Lvgl_lock(int timeout_ms);
void Lvgl_unlock(void);

/* Wake the LVGL task after an application change has invalidated the screen.
 * With this board's full-screen flush, rendering is intentionally event driven
 * instead of running on an idle cadence. */
void Lvgl_RequestRender(int id);

/* Render the invalidated screen before returning. Call only after releasing
 * Lvgl_lock(); this is for flows that must not accept another request while the
 * panel is still refreshing. */
void Lvgl_RenderNow(int id);
