/*
 * Copyright 2025 NXP
 * NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
 * accordance with the applicable license terms. By expressly accepting such terms or by
 * downloading, installing, activating and/or otherwise using the software, you are agreeing that
 * you agree to comply with those terms.
 */

#include "lvgl.h"
#include "wifi_setup_page.h"

static lv_obj_t* wifi_setup_page_create_label(lv_obj_t* parent, const char* text, int x, int y,
                                              int width, int height, const lv_font_t* font) {
  lv_obj_t* label = lv_label_create(parent);
  lv_label_set_text(label, text);
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_pos(label, x, y);
  lv_obj_set_size(label, width, height);
  lv_obj_set_style_text_color(label, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(label, font, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(label, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_pad_top(label, 4, LV_PART_MAIN | LV_STATE_DEFAULT);
  return label;
}

void wifi_setup_page_create(wifi_setup_page_t* page) {
  page->screen = lv_obj_create(NULL);
  lv_obj_set_size(page->screen, 400, 300);
  lv_obj_set_scrollbar_mode(page->screen, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_style_bg_opa(page->screen, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(page->screen, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);

  page->screen_cont_wifi_setup = lv_obj_create(page->screen);
  lv_obj_set_pos(page->screen_cont_wifi_setup, 0, 0);
  lv_obj_set_size(page->screen_cont_wifi_setup, 400, 300);
  lv_obj_set_scrollbar_mode(page->screen_cont_wifi_setup, LV_SCROLLBAR_MODE_OFF);
  lv_obj_add_flag(page->screen_cont_wifi_setup, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_border_width(page->screen_cont_wifi_setup, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(page->screen_cont_wifi_setup, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(page->screen_cont_wifi_setup, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(page->screen_cont_wifi_setup, lv_color_hex(0xffffff),
                            LV_PART_MAIN | LV_STATE_DEFAULT);

  page->screen_label_cfg_title = wifi_setup_page_create_label(
      page->screen_cont_wifi_setup, "Wi-Fi Setup", 0, 6, 400, 34, &lv_font_MISANSMEDIUM_25);
  page->screen_label_cfg_state = wifi_setup_page_create_label(
      page->screen_cont_wifi_setup, "Long-press KEY\nto enable hotspot", 10, 44, 380, 104,
      &lv_font_MISANSMEDIUM_25);
  page->screen_label_cfg_ap = wifi_setup_page_create_label(
      page->screen_cont_wifi_setup, "Hotspot disabled\nLong-press BOOT to exit", 10, 152, 380, 86,
      &lv_font_MISANSMEDIUM_18);
  page->screen_label_cfg_hint =
      wifi_setup_page_create_label(page->screen_cont_wifi_setup, "Long-press BOOT to exit", 10, 244,
                                   380, 48, &lv_font_MISANSMEDIUM_18);

  lv_obj_update_layout(page->screen);
}
