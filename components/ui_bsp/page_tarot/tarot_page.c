#include "tarot_page.h"

static lv_obj_t* tarot_root = NULL;
static lv_obj_t* tarot_img = NULL;
static lv_obj_t* tarot_msg = NULL;

void tarot_page_init(lv_obj_t* parent) {
  tarot_root = lv_obj_create(parent);
  lv_obj_set_pos(tarot_root, 0, 0);
  lv_obj_set_size(tarot_root, 400, 300);
  lv_obj_set_scrollbar_mode(tarot_root, LV_SCROLLBAR_MODE_OFF);
  lv_obj_add_flag(tarot_root, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_border_width(tarot_root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_radius(tarot_root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_opa(tarot_root, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_bg_color(tarot_root, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);

  tarot_img = lv_img_create(tarot_root);
  lv_obj_clear_flag(tarot_img, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(tarot_img, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_flag(tarot_img, LV_OBJ_FLAG_HIDDEN);

  tarot_msg = lv_label_create(tarot_root);
  lv_label_set_text(tarot_msg, "");
  lv_obj_set_style_text_color(tarot_msg, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(tarot_msg, &lv_font_MISANSMEDIUM_18,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_align(tarot_msg, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_flag(tarot_msg, LV_OBJ_FLAG_HIDDEN);

  lv_obj_update_layout(tarot_root);
}

lv_obj_t* tarot_page_root(void) { return tarot_root; }

void tarot_page_set_image(const lv_img_dsc_t* dsc) {
  if (tarot_img == NULL) {
    return;
  }
  lv_img_set_src(tarot_img, dsc);
  /* src sets the object size, so re-centre it (align is a one-off, not sticky). */
  lv_obj_align(tarot_img, LV_ALIGN_CENTER, 0, 0);
  lv_obj_clear_flag(tarot_img, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(tarot_msg, LV_OBJ_FLAG_HIDDEN);
  lv_obj_invalidate(tarot_img);
}

void tarot_page_set_message(const char* text) {
  if (tarot_msg == NULL) {
    return;
  }
  lv_label_set_text(tarot_msg, text);
  lv_obj_clear_flag(tarot_msg, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(tarot_img, LV_OBJ_FLAG_HIDDEN);
  lv_obj_invalidate(tarot_msg);
}
