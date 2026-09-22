/*******************************************************************************
 * Size: 12 px, Bpp: 1. Noto Sans CJK Bold subset for 2026--2030 year 干支.
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
    /* 丁 */ 0x00, 0x00, 0x00, 0x7F, 0xE7, 0xFE, 0x06, 0x00, 0x60, 0x06, 0x00,
    0x60,          0x06, 0x00, 0x60, 0x06, 0x01, 0xE0, 0x1E, 0x00, 0x00,
    /* 丙 */ 0x00, 0x07, 0xFE, 0x06, 0x00, 0x60, 0x7F, 0xE6, 0x66, 0x66, 0x66,
    0xFE,          0x79, 0xE7, 0x0E, 0x60, 0x66, 0x0E, 0x00, 0x00, 0x00,
    /* 午 */ 0x10, 0x03, 0x00, 0x3F, 0xC6, 0x60, 0xC6, 0x00, 0x60, 0x7F, 0xE0,
    0x60,          0x06, 0x00, 0x60, 0x06, 0x00, 0x60, 0x00, 0x00, 0x00,
    /* 己 */ 0x00, 0x03, 0xFC, 0x00, 0x40, 0x04, 0x00, 0x43, 0xFC, 0x20, 0x42,
    0x00,          0x20, 0x22, 0x03, 0x30, 0x63, 0xFE, 0x00, 0x00, 0x00,
    /* 年 */ 0x10, 0x03, 0x00, 0x3F, 0xE6, 0x20, 0x42, 0x03, 0xFE, 0x32, 0x03,
    0x20,          0xFF, 0xF0, 0x20, 0x02, 0x00, 0x20, 0x00, 0x00, 0x00,
    /* 庚 */ 0x02, 0x07, 0xFE, 0x60, 0x07, 0xFE, 0x62, 0x67, 0xFF, 0x62, 0x66,
    0x26,          0x7F, 0xE4, 0x70, 0x4D, 0xCD, 0x86, 0x00, 0x00, 0x00,
    /* 戊 */ 0x02, 0xC0, 0x26, 0x02, 0x07, 0xFE, 0x62, 0x06, 0x36, 0x63, 0x46,
    0x1C,          0x61, 0xA6, 0x3B, 0x4E, 0xE4, 0xC6, 0x00, 0x00, 0x00,
    /* 戌 */ 0x03, 0xC0, 0x36, 0x3F, 0xE2, 0x30, 0x23, 0x62, 0x16, 0x3D, 0x46,
    0x1C,          0x61, 0x86, 0x3B, 0x46, 0xE4, 0x46, 0x00, 0x00, 0x00,
    /* 未 */ 0x06, 0x00, 0x60, 0x3F, 0xE0, 0x60, 0x06, 0x07, 0xFE, 0x0E, 0x00,
    0xF0,          0x1F, 0x87, 0x6E, 0x66, 0x60, 0x60, 0x00, 0x00, 0x00,
    /* 申 */ 0x06, 0x00, 0x60, 0x7F, 0xE6, 0x66, 0x66, 0x67, 0xFE, 0x66, 0x67,
    0xFE,          0x66, 0x60, 0x60, 0x06, 0x00, 0x60, 0x00, 0x00, 0x00,
    /* 酉 */ 0x00, 0x07, 0xFE, 0x09, 0x00, 0x90, 0x7F, 0xE6, 0x96, 0x79, 0xE7,
    0x06,          0x7F, 0xE6, 0x06, 0x7F, 0xE6, 0x06, 0x00, 0x00, 0x00,
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
    {.bitmap_index = 126, .adv_w = 192, .box_w = 12, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 147, .adv_w = 192, .box_w = 12, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 168, .adv_w = 192, .box_w = 12, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 189, .adv_w = 192, .box_w = 12, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 210, .adv_w = 192, .box_w = 12, .box_h = 14, .ofs_x = 0, .ofs_y = -2},
};
static const uint16_t unicode_list_0[] = {0x0,    0x18,   0x547,  0xFF0,  0x1073, 0x1099,
                                          0x1409, 0x140B, 0x1929, 0x2732, 0x4348};
static const lv_font_fmt_txt_cmap_t cmaps[] = {{.range_start = 0x4E01,
                                                .range_length = 0x4349,
                                                .glyph_id_start = 1,
                                                .unicode_list = unicode_list_0,
                                                .glyph_id_ofs_list = NULL,
                                                .list_length = 11,
                                                .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY}};
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
const lv_font_t lv_font_calendar_ganzhi_12 = {.get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,
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
                                              .dsc = &font_dsc};
