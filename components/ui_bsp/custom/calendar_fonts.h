#ifndef CALENDAR_FONTS_H
#define CALENDAR_FONTS_H

#include "lvgl.h"

/* 1 bpp DejaVu Sans Condensed Bold, generated with lv_font_conv into
   custom/fonts/. 1 bpp because the panel is 1 bit deep: anti-aliased edges
   collapse at the flush threshold anyway, so the grey ramps would only cost
   flash. Regenerate with the command recorded at the top of each .c file. */
LV_FONT_DECLARE(lv_font_calendar_clock_48) /* digits, ':' and '-' only */
LV_FONT_DECLARE(lv_font_calendar_18)       /* ASCII + U+00B0 degree, U+00B7 middle dot */
LV_FONT_DECLARE(lv_font_calendar_16)
LV_FONT_DECLARE(lv_font_calendar_12)

#endif
