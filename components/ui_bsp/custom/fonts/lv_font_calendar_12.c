/*******************************************************************************
 * Size: 12 px
 * Bpp: 1
 * Opts: --font /usr/share/fonts/truetype/dejavu/DejaVuSansCondensed-Bold.ttf --size 12 --bpp 1 --format lvgl -r 0x20-0x7E -r 0xB0 -r 0xB7 --force-fast-kern-format --no-compress --lv-include lvgl.h -o components/ui_bsp/custom/fonts/lv_font_calendar_12.c
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifndef LV_FONT_CALENDAR_12
#define LV_FONT_CALENDAR_12 1
#endif

#if LV_FONT_CALENDAR_12

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */
    0x0,

    /* U+0021 "!" */
    0xff, 0xf0, 0xc0,

    /* U+0022 "\"" */
    0xb6, 0x80,

    /* U+0023 "#" */
    0x16, 0x14, 0x7f, 0x24, 0x24, 0xfe, 0x28, 0x28,
    0x68,

    /* U+0024 "$" */
    0x21, 0xee, 0x38, 0xf3, 0xf3, 0xcb, 0xaf, 0xe2,
    0x0,

    /* U+0025 "%" */
    0x71, 0x36, 0x8d, 0xa3, 0x70, 0x75, 0x82, 0xf0,
    0xbc, 0x4f, 0x11, 0x80,

    /* U+0026 "&" */
    0x3c, 0x32, 0x18, 0x6, 0x7, 0x36, 0xdb, 0x3d,
    0x9c, 0x7f, 0x0,

    /* U+0027 "'" */
    0xe0,

    /* U+0028 "(" */
    0x6d, 0x6d, 0xb6, 0x4d, 0x80,

    /* U+0029 ")" */
    0xd9, 0x36, 0xdb, 0x5b, 0x0,

    /* U+002A "*" */
    0x25, 0x5c, 0xea, 0x90,

    /* U+002B "+" */
    0x30, 0x60, 0xc1, 0x8f, 0xe6, 0xc, 0x18,

    /* U+002C "," */
    0x6d, 0xa0,

    /* U+002D "-" */
    0xe0,

    /* U+002E "." */
    0xc0,

    /* U+002F "/" */
    0x13, 0x22, 0x26, 0x44, 0x48, 0x80,

    /* U+0030 "0" */
    0x79, 0x2c, 0xf3, 0xcf, 0x3c, 0xd2, 0x78,

    /* U+0031 "1" */
    0xf3, 0xc3, 0xc, 0x30, 0xc3, 0xc, 0xfc,

    /* U+0032 "2" */
    0xfa, 0x30, 0xc3, 0x1c, 0xe7, 0x38, 0xfc,

    /* U+0033 "3" */
    0xfa, 0x30, 0xc3, 0x78, 0x30, 0xe3, 0xf8,

    /* U+0034 "4" */
    0x1c, 0x38, 0xf1, 0x66, 0xc9, 0xbf, 0x86, 0xc,

    /* U+0035 "5" */
    0x7c, 0xc1, 0x83, 0xe0, 0xe0, 0xc1, 0xc7, 0xfc,

    /* U+0036 "6" */
    0x39, 0xc, 0x3e, 0xcf, 0x3c, 0xd3, 0x78,

    /* U+0037 "7" */
    0xfc, 0x31, 0x86, 0x18, 0xc3, 0x18, 0x60,

    /* U+0038 "8" */
    0x7b, 0x3c, 0xf3, 0x73, 0x3c, 0xf3, 0x78,

    /* U+0039 "9" */
    0x7b, 0x2c, 0xf3, 0x7c, 0x30, 0xc6, 0x70,

    /* U+003A ":" */
    0xc0, 0xc,

    /* U+003B ";" */
    0x60, 0x36, 0xd0,

    /* U+003C "<" */
    0x6, 0x7b, 0x87, 0x3, 0x81, 0xc0, 0x0,

    /* U+003D "=" */
    0xfe, 0x0, 0x7, 0xf0,

    /* U+003E ">" */
    0xc0, 0xf0, 0x38, 0x73, 0x9c, 0x0, 0x0,

    /* U+003F "?" */
    0xf4, 0xc6, 0x73, 0x31, 0x80, 0x60,

    /* U+0040 "@" */
    0x1e, 0x31, 0x90, 0x71, 0xd9, 0x2c, 0x96, 0x4b,
    0x1e, 0x40, 0x31, 0x7, 0x80,

    /* U+0041 "A" */
    0x1c, 0x3c, 0x3c, 0x36, 0x76, 0x66, 0x7f, 0x43,
    0xc3,

    /* U+0042 "B" */
    0xfb, 0x3c, 0xf3, 0xfb, 0x3c, 0xf3, 0xf8,

    /* U+0043 "C" */
    0x3e, 0xc7, 0x6, 0xc, 0x18, 0x30, 0x31, 0x3e,

    /* U+0044 "D" */
    0xf9, 0x9b, 0x1e, 0x3c, 0x78, 0xf1, 0xe6, 0xf8,

    /* U+0045 "E" */
    0xff, 0xc, 0x30, 0xff, 0xc, 0x30, 0xfc,

    /* U+0046 "F" */
    0xff, 0xc, 0x30, 0xff, 0xc, 0x30, 0xc0,

    /* U+0047 "G" */
    0x3f, 0x61, 0xc0, 0xc0, 0xc7, 0xc3, 0xc3, 0x63,
    0x3f,

    /* U+0048 "H" */
    0xc7, 0x8f, 0x1e, 0x3f, 0xf8, 0xf1, 0xe3, 0xc6,

    /* U+0049 "I" */
    0xff, 0xff, 0xc0,

    /* U+004A "J" */
    0x33, 0x33, 0x33, 0x33, 0x33, 0x3e,

    /* U+004B "K" */
    0xce, 0xcc, 0xd8, 0xf0, 0xf0, 0xf8, 0xd8, 0xcc,
    0xc6,

    /* U+004C "L" */
    0xc3, 0xc, 0x30, 0xc3, 0xc, 0x30, 0xfc,

    /* U+004D "M" */
    0xe3, 0xf1, 0xfd, 0xfe, 0xfd, 0x5e, 0xef, 0x27,
    0x83, 0xc1, 0x80,

    /* U+004E "N" */
    0xe7, 0xcf, 0x9f, 0xbd, 0x7b, 0xf3, 0xe7, 0xce,

    /* U+004F "O" */
    0x3c, 0x66, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x66,
    0x3c,

    /* U+0050 "P" */
    0xfb, 0x3c, 0xf3, 0xfb, 0xc, 0x30, 0xc0,

    /* U+0051 "Q" */
    0x3c, 0x66, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x66,
    0x3c, 0x6, 0x2,

    /* U+0052 "R" */
    0xf9, 0x9b, 0x36, 0x6f, 0x9b, 0xb3, 0x66, 0xc6,

    /* U+0053 "S" */
    0x7b, 0xc, 0x3e, 0xfc, 0xf0, 0xe3, 0xf8,

    /* U+0054 "T" */
    0xfe, 0x30, 0x60, 0xc1, 0x83, 0x6, 0xc, 0x18,

    /* U+0055 "U" */
    0xc7, 0x8f, 0x1e, 0x3c, 0x78, 0xf1, 0xe3, 0x7c,

    /* U+0056 "V" */
    0xc3, 0x63, 0x67, 0x66, 0x36, 0x36, 0x3c, 0x3c,
    0x1c,

    /* U+0057 "W" */
    0xc6, 0x26, 0x66, 0x6e, 0x66, 0xf6, 0x6b, 0x66,
    0x9c, 0x39, 0xc3, 0x9c, 0x39, 0xc0,

    /* U+0058 "X" */
    0x63, 0x66, 0x3e, 0x3c, 0x18, 0x3c, 0x36, 0x66,
    0xe3,

    /* U+0059 "Y" */
    0xe7, 0x66, 0x7e, 0x3c, 0x18, 0x18, 0x18, 0x18,
    0x18,

    /* U+005A "Z" */
    0xfe, 0x1c, 0x70, 0xc3, 0x86, 0x1c, 0x70, 0xfe,

    /* U+005B "[" */
    0xfb, 0x6d, 0xb6, 0xdb, 0x80,

    /* U+005C "\\" */
    0x8c, 0x44, 0x46, 0x22, 0x21, 0x10,

    /* U+005D "]" */
    0xed, 0xb6, 0xdb, 0x6f, 0x80,

    /* U+005E "^" */
    0x10, 0x71, 0xb6, 0x30,

    /* U+005F "_" */
    0xf8,

    /* U+0060 "`" */
    0xcc, 0x80,

    /* U+0061 "a" */
    0x78, 0x30, 0xdf, 0xcf, 0x37, 0xc0,

    /* U+0062 "b" */
    0xc3, 0xc, 0x3e, 0xcf, 0x3c, 0xf3, 0xcf, 0xe0,

    /* U+0063 "c" */
    0x7f, 0x31, 0x8c, 0x71, 0xe0,

    /* U+0064 "d" */
    0xc, 0x30, 0xdf, 0xcf, 0x3c, 0xf3, 0xcd, 0xf0,

    /* U+0065 "e" */
    0x7b, 0x3c, 0xff, 0xc3, 0x17, 0xc0,

    /* U+0066 "f" */
    0x36, 0x6f, 0x66, 0x66, 0x66,

    /* U+0067 "g" */
    0x7f, 0x3c, 0xf3, 0xcf, 0x37, 0xc3, 0x8d, 0xe0,

    /* U+0068 "h" */
    0xc3, 0xc, 0x3e, 0xcf, 0x3c, 0xf3, 0xcf, 0x30,

    /* U+0069 "i" */
    0xc3, 0xff, 0xf0,

    /* U+006A "j" */
    0x30, 0x3, 0x33, 0x33, 0x33, 0x33, 0xe0,

    /* U+006B "k" */
    0xc1, 0x83, 0x6, 0x6d, 0x9e, 0x3c, 0x7c, 0xd9,
    0x98,

    /* U+006C "l" */
    0xff, 0xff, 0xf0,

    /* U+006D "m" */
    0xfb, 0xb3, 0x3c, 0xcf, 0x33, 0xcc, 0xf3, 0x3c,
    0xcc,

    /* U+006E "n" */
    0xfb, 0x3c, 0xf3, 0xcf, 0x3c, 0xc0,

    /* U+006F "o" */
    0x7b, 0x3c, 0xf3, 0xcf, 0x37, 0x80,

    /* U+0070 "p" */
    0xfb, 0x3c, 0xf3, 0xcf, 0x3f, 0xb0, 0xc3, 0x0,

    /* U+0071 "q" */
    0x7f, 0x3c, 0xf3, 0xcf, 0x37, 0xc3, 0xc, 0x30,

    /* U+0072 "r" */
    0xff, 0x31, 0x8c, 0x63, 0x0,

    /* U+0073 "s" */
    0x7b, 0xf, 0x3e, 0x38, 0x6f, 0x80,

    /* U+0074 "t" */
    0x63, 0x3e, 0xc6, 0x31, 0x8c, 0x38,

    /* U+0075 "u" */
    0xcf, 0x3c, 0xf3, 0xcf, 0x37, 0xc0,

    /* U+0076 "v" */
    0xc6, 0xc9, 0xb3, 0x62, 0x87, 0xe, 0x0,

    /* U+0077 "w" */
    0xcc, 0xdb, 0x66, 0xd9, 0xfe, 0x7f, 0x8c, 0xc3,
    0x30,

    /* U+0078 "x" */
    0x6c, 0xd8, 0xe1, 0xc3, 0x8d, 0xb1, 0x0,

    /* U+0079 "y" */
    0xc6, 0x99, 0xb3, 0x62, 0x87, 0xe, 0x8, 0x30,
    0xc0,

    /* U+007A "z" */
    0xf8, 0xcc, 0xe6, 0x63, 0xe0,

    /* U+007B "{" */
    0x1c, 0xc3, 0xc, 0x33, 0x83, 0xc, 0x30, 0xc1,
    0xc0,

    /* U+007C "|" */
    0xff, 0xf0,

    /* U+007D "}" */
    0xe0, 0xc3, 0xc, 0x30, 0x73, 0xc, 0x30, 0xce,
    0x0,

    /* U+007E "~" */
    0x73, 0x38,

    /* U+00B0 "°" */
    0x69, 0x96,

    /* U+00B7 "·" */
    0xc0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 60, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1, .adv_w = 79, .box_w = 2, .box_h = 9, .ofs_x = 2, .ofs_y = 0},
    {.bitmap_index = 4, .adv_w = 90, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 6, .adv_w = 145, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 15, .adv_w = 120, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 24, .adv_w = 173, .box_w = 10, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 36, .adv_w = 151, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 47, .adv_w = 53, .box_w = 1, .box_h = 3, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 48, .adv_w = 79, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 53, .adv_w = 79, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 58, .adv_w = 90, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 62, .adv_w = 145, .box_w = 7, .box_h = 8, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 69, .adv_w = 66, .box_w = 3, .box_h = 4, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 71, .adv_w = 72, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 72, .adv_w = 66, .box_w = 2, .box_h = 1, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 73, .adv_w = 63, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 79, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 86, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 93, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 100, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 107, .adv_w = 120, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 115, .adv_w = 120, .box_w = 7, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 123, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 130, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 137, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 144, .adv_w = 120, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 151, .adv_w = 69, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 153, .adv_w = 69, .box_w = 3, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 156, .adv_w = 145, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 163, .adv_w = 145, .box_w = 7, .box_h = 4, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 167, .adv_w = 145, .box_w = 7, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 174, .adv_w = 100, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 180, .adv_w = 173, .box_w = 9, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 193, .adv_w = 134, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 202, .adv_w = 132, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 209, .adv_w = 127, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 217, .adv_w = 143, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 225, .adv_w = 118, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 232, .adv_w = 118, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 239, .adv_w = 142, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 248, .adv_w = 145, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 256, .adv_w = 64, .box_w = 2, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 259, .adv_w = 64, .box_w = 4, .box_h = 12, .ofs_x = -1, .ofs_y = -3},
    {.bitmap_index = 265, .adv_w = 134, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 274, .adv_w = 110, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 281, .adv_w = 172, .box_w = 9, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 292, .adv_w = 145, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 300, .adv_w = 147, .box_w = 8, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 309, .adv_w = 127, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 316, .adv_w = 147, .box_w = 8, .box_h = 11, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 327, .adv_w = 133, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 335, .adv_w = 124, .box_w = 6, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 342, .adv_w = 118, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 350, .adv_w = 140, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 358, .adv_w = 134, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 367, .adv_w = 191, .box_w = 12, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 381, .adv_w = 133, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 390, .adv_w = 125, .box_w = 8, .box_h = 9, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 399, .adv_w = 125, .box_w = 7, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 407, .adv_w = 79, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 412, .adv_w = 63, .box_w = 4, .box_h = 11, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 418, .adv_w = 79, .box_w = 3, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 423, .adv_w = 145, .box_w = 7, .box_h = 4, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 427, .adv_w = 86, .box_w = 5, .box_h = 1, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 428, .adv_w = 86, .box_w = 3, .box_h = 3, .ofs_x = 0, .ofs_y = 8},
    {.bitmap_index = 430, .adv_w = 117, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 436, .adv_w = 124, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 444, .adv_w = 102, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 449, .adv_w = 124, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 457, .adv_w = 117, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 463, .adv_w = 75, .box_w = 4, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 468, .adv_w = 124, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 476, .adv_w = 123, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 484, .adv_w = 59, .box_w = 2, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 487, .adv_w = 59, .box_w = 4, .box_h = 13, .ofs_x = -1, .ofs_y = -3},
    {.bitmap_index = 494, .adv_w = 115, .box_w = 7, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 503, .adv_w = 59, .box_w = 2, .box_h = 10, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 506, .adv_w = 180, .box_w = 10, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 515, .adv_w = 123, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 521, .adv_w = 119, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 527, .adv_w = 124, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 535, .adv_w = 124, .box_w = 6, .box_h = 10, .ofs_x = 1, .ofs_y = -3},
    {.bitmap_index = 543, .adv_w = 85, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 548, .adv_w = 103, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 554, .adv_w = 83, .box_w = 5, .box_h = 9, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 560, .adv_w = 123, .box_w = 6, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 566, .adv_w = 113, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 573, .adv_w = 160, .box_w = 10, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 582, .adv_w = 111, .box_w = 7, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 589, .adv_w = 113, .box_w = 7, .box_h = 10, .ofs_x = 0, .ofs_y = -3},
    {.bitmap_index = 598, .adv_w = 101, .box_w = 5, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 603, .adv_w = 123, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 612, .adv_w = 63, .box_w = 1, .box_h = 12, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 614, .adv_w = 123, .box_w = 6, .box_h = 11, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 623, .adv_w = 145, .box_w = 7, .box_h = 2, .ofs_x = 1, .ofs_y = 3},
    {.bitmap_index = 625, .adv_w = 86, .box_w = 4, .box_h = 4, .ofs_x = 1, .ofs_y = 5},
    {.bitmap_index = 627, .adv_w = 66, .box_w = 2, .box_h = 1, .ofs_x = 1, .ofs_y = 4}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

static const uint16_t unicode_list_1[] = {
    0x0, 0x7
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    },
    {
        .range_start = 176, .range_length = 8, .glyph_id_start = 96,
        .unicode_list = unicode_list_1, .glyph_id_ofs_list = NULL, .list_length = 2, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LV_VERSION_CHECK(8, 0, 0)
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 2,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LV_VERSION_CHECK(8, 0, 0)
    .cache = &cache
#endif
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LV_VERSION_CHECK(8, 0, 0)
const lv_font_t lv_font_calendar_12 = {
#else
lv_font_t lv_font_calendar_12 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 14,          /*The maximum line height required by the font*/
    .base_line = 3,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = 0,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc           /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
};



#endif /*#if LV_FONT_CALENDAR_12*/
