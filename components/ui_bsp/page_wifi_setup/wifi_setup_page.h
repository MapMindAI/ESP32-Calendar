/*
 * Copyright 2025 NXP
 * NXP Proprietary. This software is owned or controlled by NXP and may only be used strictly in
 * accordance with the applicable license terms. By expressly accepting such terms or by
 * downloading, installing, activating and/or otherwise using the software, you are agreeing that
 * you have read, and that you agree to comply with and are bound by, such license terms.  If you do
 * not agree to be bound by the applicable license terms, then you may not retain, install, activate
 * or otherwise use the software.
 */

#ifndef WIFI_SETUP_PAGE_H
#define WIFI_SETUP_PAGE_H
#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

typedef struct {
  lv_obj_t* screen;
  lv_obj_t* screen_cont_wifi_setup;
  lv_obj_t* screen_label_cfg_title;
  lv_obj_t* screen_label_cfg_state;
  lv_obj_t* screen_label_cfg_ap;
  lv_obj_t* screen_label_cfg_hint;
} wifi_setup_page_t;

void wifi_setup_page_init(wifi_setup_page_t* page);
void wifi_setup_page_create(wifi_setup_page_t* page);
LV_FONT_DECLARE(lv_font_MISANSMEDIUM_25)
LV_FONT_DECLARE(lv_font_MISANSMEDIUM_18)

#ifdef __cplusplus
}
#endif
#endif
