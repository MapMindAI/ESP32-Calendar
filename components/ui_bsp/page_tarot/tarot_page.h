#ifndef TAROT_PAGE_H
#define TAROT_PAGE_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The tarot view: a single monochrome card, or a short status message when no
 * card can be read. ui_bsp sits below app_bsp, so none of these take the LVGL
 * lock themselves — every call must be made from the LVGL task or between
 * Lvgl_lock()/Lvgl_unlock().
 */

/* Build the tarot container as a full-screen child of `parent`, hidden. */
void tarot_page_init(lv_obj_t* parent);

/* The container, for the show/hide juggling the view switcher does. */
lv_obj_t* tarot_page_root(void);

/* Show an already-decoded image. `dsc` must stay valid while it is displayed;
 * the caller owns its pixel buffer. */
void tarot_page_set_image(const lv_img_dsc_t* dsc);

/* Show a status line instead of an image ("No SD card", ...). */
void tarot_page_set_message(const char* text);

LV_FONT_DECLARE(lv_font_MISANSMEDIUM_18)

#ifdef __cplusplus
}
#endif

#endif
