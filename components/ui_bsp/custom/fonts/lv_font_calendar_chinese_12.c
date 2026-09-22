/*******************************************************************************
 * Size: 12 px, Bpp: 1
 * Source: Noto Sans CJK Bold.  Subset: 今 吉 日 道 非 黄.
 *
 * The dashboard has a 1-bit panel, so this deliberately stores only monochrome
 * glyphs.  The six-glyph subset is sufficient for 今日黄道吉日 / 今日非黄道日.
 *
 * LVGL reads a 1 bpp glyph as one MSB-first bit stream whose row stride is
 * box_w bits -- rows are not padded to a byte boundary, so each 12 x 14 glyph
 * is 21 contiguous bytes.
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+4ECA "今" */
    0x06,
    0x00,
    0xF0,
    0x1D,
    0x83,
    0xCC,
    0x66,
    0x64,
    0x32,
    0x3F,
    0xE0,
    0x0C,
    0x01,
    0x80,
    0x18,
    0x03,
    0x00,
    0x20,
    0x00,
    0x00,
    0x00,
    /* U+5409 "吉" */
    0x06,
    0x00,
    0x60,
    0x7F,
    0xE0,
    0x60,
    0x06,
    0x07,
    0xFE,
    0x00,
    0x03,
    0xFC,
    0x20,
    0x42,
    0x04,
    0x3F,
    0xC2,
    0x04,
    0x00,
    0x00,
    0x00,
    /* U+65E5 "日" */
    0x00,
    0x03,
    0xFC,
    0x20,
    0x42,
    0x04,
    0x20,
    0x43,
    0xFC,
    0x20,
    0x42,
    0x04,
    0x20,
    0x42,
    0x04,
    0x3F,
    0xC2,
    0x04,
    0x00,
    0x00,
    0x00,
    /* U+9053 "道" */
    0x04,
    0x44,
    0x6C,
    0x6F,
    0xE0,
    0x30,
    0x07,
    0xE6,
    0x7E,
    0x24,
    0x62,
    0x7E,
    0x24,
    0x62,
    0x7E,
    0x78,
    0x14,
    0xFE,
    0x00,
    0x00,
    0x00,
    /* U+975E "非" */
    0x09,
    0x00,
    0x90,
    0x79,
    0xE0,
    0x90,
    0x09,
    0x07,
    0x9E,
    0x09,
    0x00,
    0x90,
    0x79,
    0xF0,
    0x90,
    0x09,
    0x00,
    0x90,
    0x00,
    0x00,
    0x00,
    /* U+9EC4 "黄" */
    0x19,
    0x87,
    0xFE,
    0x19,
    0x87,
    0xFE,
    0x06,
    0x03,
    0xFC,
    0x26,
    0x43,
    0xFC,
    0x26,
    0x43,
    0xFC,
    0x19,
    0xC7,
    0x06,
    0x00,
    0x00,
    0x00,
};

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    /* Index 0 is the empty placeholder lv_font_conv emits: LVGL indexes
       glyph_dsc[glyph_id_start + ofs], and glyph_id_start is 1. */
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 0, .adv_w = 192, .box_w = 12, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 21, .adv_w = 192, .box_w = 12, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 42, .adv_w = 192, .box_w = 12, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 63, .adv_w = 192, .box_w = 12, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 84, .adv_w = 192, .box_w = 12, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 105, .adv_w = 192, .box_w = 12, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
};

static const uint16_t unicode_list_0[] = {0x0, 0x53F, 0x171B, 0x4189, 0x4894, 0x4FFA};

static const lv_font_fmt_txt_cmap_t cmaps[] = {
    {.range_start = 0x4ECA,
     .range_length = 0x4FFB,
     .glyph_id_start = 1,
     .unicode_list = unicode_list_0,
     .glyph_id_ofs_list = NULL,
     .list_length = 6,
     .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY},
};

#if LV_VERSION_CHECK(8, 0, 0)
static lv_font_fmt_txt_glyph_cache_t cache;
#endif
static const lv_font_fmt_txt_dsc_t font_dsc = {
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LV_VERSION_CHECK(8, 0, 0)
    .cache = &cache,
#endif
};

const lv_font_t lv_font_calendar_chinese_12 = {
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,
    .line_height = 14,
    .base_line = 2,
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = 0,
    .underline_thickness = 1,
#endif
    .dsc = &font_dsc,
};
