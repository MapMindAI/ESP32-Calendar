#include "tarot_page.h"

#define TAROT_MARGIN -8
#define TAROT_GAP 0
#define TAROT_IMG_Y 14
#define TAROT_LABEL_Y (TAROT_IMG_Y + TAROT_CARD_HEIGHT + 4)
#define TAROT_LABEL_HEIGHT 46

static lv_obj_t* tarot_root = NULL;
static lv_obj_t* tarot_img[TAROT_CARD_SLOTS];
static lv_obj_t* tarot_name[TAROT_CARD_SLOTS];
static lv_obj_t* tarot_msg = NULL;

/* Columns are laid out from the left: margin, card, gap, card, gap, card, margin. */
static int tarot_card_x(int slot) {
  return TAROT_MARGIN + slot * (TAROT_CARD_WIDTH + TAROT_GAP);
}

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

  for (int slot = 0; slot < TAROT_CARD_SLOTS; slot++) {
    tarot_img[slot] = lv_img_create(tarot_root);
    lv_obj_clear_flag(tarot_img[slot], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(tarot_img[slot], tarot_card_x(slot), TAROT_IMG_Y);
    lv_obj_add_flag(tarot_img[slot], LV_OBJ_FLAG_HIDDEN);

    tarot_name[slot] = lv_label_create(tarot_root);
    lv_label_set_text(tarot_name[slot], "");
    lv_label_set_long_mode(tarot_name[slot], LV_LABEL_LONG_WRAP);
    lv_obj_set_pos(tarot_name[slot], tarot_card_x(slot), TAROT_LABEL_Y);
    lv_obj_set_size(tarot_name[slot], TAROT_CARD_WIDTH, TAROT_LABEL_HEIGHT);
    lv_obj_set_style_text_align(tarot_name[slot], LV_TEXT_ALIGN_CENTER,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(tarot_name[slot], lv_color_hex(0x000000),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(tarot_name[slot], &lv_font_calendar_12,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(tarot_name[slot], LV_OBJ_FLAG_HIDDEN);
  }

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

void tarot_page_set_card(int slot, const lv_img_dsc_t* dsc, const char* name) {
  if (slot < 0 || slot >= TAROT_CARD_SLOTS || tarot_img[slot] == NULL) {
    return;
  }
  if (dsc == NULL) {
    lv_obj_add_flag(tarot_img[slot], LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(tarot_name[slot], LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_img_set_src(tarot_img[slot], dsc);
  lv_obj_clear_flag(tarot_img[slot], LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(tarot_name[slot], name != NULL ? name : "");
  lv_obj_clear_flag(tarot_name[slot], LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(tarot_msg, LV_OBJ_FLAG_HIDDEN);
  lv_obj_invalidate(tarot_root);
}

void tarot_page_set_message(const char* text) {
  if (tarot_msg == NULL) {
    return;
  }
  for (int slot = 0; slot < TAROT_CARD_SLOTS; slot++) {
    lv_obj_add_flag(tarot_img[slot], LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(tarot_name[slot], LV_OBJ_FLAG_HIDDEN);
  }
  lv_label_set_text(tarot_msg, text);
  lv_obj_clear_flag(tarot_msg, LV_OBJ_FLAG_HIDDEN);
  lv_obj_invalidate(tarot_root);
}
