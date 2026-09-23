#ifndef TAROT_PAGE_H
#define TAROT_PAGE_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The tarot view: three random cards in a row, each with its name underneath, or
 * a short status message when no card can be read. ui_bsp sits below app_bsp, so
 * none of these take the LVGL lock themselves — every call must be made from the
 * LVGL task or between Lvgl_lock()/Lvgl_unlock().
 */

/* Cards shown at once, one slot per column. */
#define TAROT_CARD_SLOTS 3

/* On-screen size of one card. Kept here so the manager resamples to exactly what
 * the page lays out. */
#define TAROT_CARD_WIDTH 128
#define TAROT_CARD_HEIGHT 219

/* Build the tarot container as a full-screen child of `parent`, hidden. */
void tarot_page_init(lv_obj_t* parent);

/* The container, for the show/hide juggling the view switcher does. */
lv_obj_t* tarot_page_root(void);

/* Show `dsc` and `name` in `slot` (0 … TAROT_CARD_SLOTS-1). Passing a NULL `dsc`
 * hides the slot. `dsc` must stay valid while it is displayed; the caller owns
 * its pixel buffer. */
void tarot_page_set_card(int slot, const lv_img_dsc_t* dsc, const char* name);

/* Show a status line instead of the cards ("No SD card", ...). */
void tarot_page_set_message(const char* text);

LV_FONT_DECLARE(lv_font_MISANSMEDIUM_18)
LV_FONT_DECLARE(lv_font_calendar_12)

#ifdef __cplusplus
}
#endif

#endif
