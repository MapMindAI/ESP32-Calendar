#include "tarot_manager.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include <esp_heap_caps.h>
#include <esp_log.h>
#include <esp_random.h>

#include "esp_jpeg_dec.h"
#include "lvgl.h"
#include "lvgl_bsp.h"
#include "sdcard_bsp.h"
#include "tarot_names.h"
#include "tarot_page.h"

/* Cards are 350 x 600; each is resampled to TAROT_CARD_WIDTH x TAROT_CARD_HEIGHT
 * (from tarot_page.h) so the manager produces exactly what the page lays out. */
#define TAROT_DIR "/sdcard/tarot/images"
#define TAROT_MAX_FILES 96
#define TAROT_NAME_LEN 20
#define TAROT_LABEL_LEN 24

static const char* TAG = "tarot";

/* 4 x 4 dispersed-dot threshold, used as an ordered-dither halftone so a
 * detailed colour card stays legible on the 1-bit panel. */
static const uint8_t bayer4[4][4] = {
    {0, 8, 2, 10},
    {12, 4, 14, 6},
    {3, 11, 1, 9},
    {15, 7, 13, 5},
};

static CustomSDPort* sd_port = nullptr;
static char file_names[TAROT_MAX_FILES][TAROT_NAME_LEN];
static char file_labels[TAROT_MAX_FILES][TAROT_LABEL_LEN];
static int file_count = 0;

/* One descriptor per column. Each is stable; its pixel buffer is swapped when a
 * new set of cards is drawn. */
static lv_img_dsc_t card_dsc[TAROT_CARD_SLOTS];
static lv_color_t* card_pixels[TAROT_CARD_SLOTS] = {};

static bool sd_ready(void) {
  if (sd_port != nullptr && sd_port->SDPort_GetStatus()) {
    return true;
  }
  /* Nothing mounted yet, or the card went away: retry, so a card inserted after
   * boot is picked up the next time the view is opened. */
  delete sd_port;
  sd_port = new CustomSDPort("/sdcard");
  return sd_port->SDPort_GetStatus();
}

static void label_for(const char* file, char* out, size_t out_len) {
  for (size_t i = 0; i < TAROT_CARD_NAME_COUNT; i++) {
    if (strcasecmp(tarot_card_names[i].file, file) == 0) {
      strncpy(out, tarot_card_names[i].name, out_len - 1);
      out[out_len - 1] = '\0';
      return;
    }
  }
  /* Unknown file: fall back to its name without the extension. */
  strncpy(out, file, out_len - 1);
  out[out_len - 1] = '\0';
  char* dot = strrchr(out, '.');
  if (dot != nullptr) {
    *dot = '\0';
  }
}

static bool scan_files(void) {
  file_count = 0;
  DIR* dir = opendir(TAROT_DIR);
  if (dir == nullptr) {
    return false;
  }
  struct dirent* entry;
  while ((entry = readdir(dir)) != nullptr && file_count < TAROT_MAX_FILES) {
    const char* name = entry->d_name;
    size_t len = strlen(name);
    if (len < 5 || len >= TAROT_NAME_LEN) {
      continue;
    }
    if (strcasecmp(name + len - 4, ".jpg") != 0) {
      continue;
    }
    strcpy(file_names[file_count], name);
    label_for(name, file_labels[file_count], sizeof(file_labels[0]));
    file_count++;
  }
  closedir(dir);
  return file_count > 0;
}

static uint32_t pixel_luma(uint16_t rgb565) {
  int r = (rgb565 >> 11) & 0x1f;
  int g = (rgb565 >> 5) & 0x3f;
  int b = rgb565 & 0x1f;
  int r8 = (r * 255) / 31;
  int g8 = (g * 255) / 63;
  int b8 = (b * 255) / 31;
  return (uint32_t)((r8 * 299 + g8 * 587 + b8 * 114) / 1000);
}

/* Decode a JPEG file to a 16-byte aligned RGB565 buffer in PSRAM. The caller
 * owns *out_rgb and frees it with heap_caps_free(). */
static bool decode_rgb565(const char* path, uint8_t** out_rgb, uint16_t* out_w, uint16_t* out_h) {
  *out_rgb = nullptr;

  FILE* f = fopen(path, "rb");
  if (f == nullptr) {
    ESP_LOGE(TAG, "open failed: %s", path);
    return false;
  }
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (size <= 0) {
    ESP_LOGE(TAG, "empty file: %s", path);
    fclose(f);
    return false;
  }

  uint8_t* inbuf = (uint8_t*)heap_caps_malloc((size_t)size, MALLOC_CAP_SPIRAM);
  if (inbuf == nullptr) {
    ESP_LOGE(TAG, "no PSRAM for %ld input bytes", size);
    fclose(f);
    return false;
  }
  size_t read = fread(inbuf, 1, (size_t)size, f);
  fclose(f);
  if (read != (size_t)size) {
    heap_caps_free(inbuf);
    return false;
  }

  /* Plain member assignments instead of DEFAULT_JPEG_DEC_CONFIG(): the macro's
   * nested designated initializers are not valid C++. */
  jpeg_dec_config_t config = {};
  config.output_type = JPEG_PIXEL_FORMAT_RGB565_LE; /* matches LV_COLOR_16_SWAP=0 */
  config.rotate = JPEG_ROTATE_0D;
  config.block_enable = false;

  jpeg_dec_handle_t dec = nullptr;
  if (jpeg_dec_open(&config, &dec) != JPEG_ERR_OK) {
    ESP_LOGE(TAG, "decoder open failed: %s", path);
    heap_caps_free(inbuf);
    return false;
  }

  jpeg_dec_io_t io = {};
  io.inbuf = inbuf;
  io.inbuf_len = (int)size;
  jpeg_dec_header_info_t info = {};
  if (jpeg_dec_parse_header(dec, &io, &info) != JPEG_ERR_OK) {
    ESP_LOGE(TAG, "header parse failed: %s", path);
    jpeg_dec_close(dec);
    heap_caps_free(inbuf);
    return false;
  }

  int out_len = 0;
  if (jpeg_dec_get_outbuf_len(dec, &out_len) != JPEG_ERR_OK || out_len <= 0) {
    jpeg_dec_close(dec);
    heap_caps_free(inbuf);
    return false;
  }
  /* Not jpeg_calloc_align(): that may land in internal RAM, which cannot hold
   * a full card. The decoder only needs 16-byte alignment. */
  uint8_t* rgb = (uint8_t*)heap_caps_aligned_alloc(16, (size_t)out_len, MALLOC_CAP_SPIRAM);
  if (rgb == nullptr) {
    ESP_LOGE(TAG, "no PSRAM for %d decoded bytes", out_len);
    jpeg_dec_close(dec);
    heap_caps_free(inbuf);
    return false;
  }

  io.outbuf = rgb;
  io.out_size = out_len;
  if (jpeg_dec_process(dec, &io) != JPEG_ERR_OK) {
    ESP_LOGE(TAG, "decode failed: %s", path);
    heap_caps_free(rgb);
    jpeg_dec_close(dec);
    heap_caps_free(inbuf);
    return false;
  }

  jpeg_dec_close(dec);
  heap_caps_free(inbuf);
  *out_rgb = rgb;
  *out_w = info.width;
  *out_h = info.height;
  return true;
}

/* Box-average each source footprint down to the on-screen card size, then reduce
 * it to black/white with an ordered dither. Pure black and white survive the
 * flush threshold unchanged, so the panel shows exactly the dot pattern chosen
 * here. */
static lv_color_t* card_to_mono(const uint8_t* rgb, uint16_t width, uint16_t height) {
  lv_color_t* pixels = (lv_color_t*)heap_caps_malloc(
      TAROT_CARD_WIDTH * TAROT_CARD_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
  if (pixels == nullptr) {
    ESP_LOGE(TAG, "no PSRAM for the mono frame");
    return nullptr;
  }

  const uint16_t* src = (const uint16_t*)rgb;
  for (int y = 0; y < TAROT_CARD_HEIGHT; y++) {
    int sy0 = (int)((uint32_t)y * height / TAROT_CARD_HEIGHT);
    int sy1 = (int)((uint32_t)(y + 1) * height / TAROT_CARD_HEIGHT);
    if (sy1 <= sy0) {
      sy1 = sy0 + 1;
    }
    for (int x = 0; x < TAROT_CARD_WIDTH; x++) {
      int sx0 = (int)((uint32_t)x * width / TAROT_CARD_WIDTH);
      int sx1 = (int)((uint32_t)(x + 1) * width / TAROT_CARD_WIDTH);
      if (sx1 <= sx0) {
        sx1 = sx0 + 1;
      }
      uint32_t sum = 0;
      int samples = 0;
      for (int sy = sy0; sy < sy1; sy++) {
        const uint16_t* row = src + (size_t)sy * width;
        for (int sx = sx0; sx < sx1; sx++) {
          sum += pixel_luma(row[sx]);
          samples++;
        }
      }
      int luma = samples ? (int)(sum / (uint32_t)samples) : 255;
      int threshold = (bayer4[y & 3][x & 3] + 1) * 16; /* 16 .. 256 */
      pixels[y * TAROT_CARD_WIDTH + x] =
          (luma >= threshold) ? lv_color_white() : lv_color_black();
    }
  }
  return pixels;
}

static void show_message(const char* text) {
  if (Lvgl_lock(-1)) {
    tarot_page_set_message(text);
    Lvgl_unlock();
    Lvgl_RequestRender(21);
  }
}

void Tarot_ManagerInit(void) {
  if (!sd_ready()) {
    ESP_LOGW(TAG, "no SD card");
    return;
  }
  if (scan_files()) {
    ESP_LOGI(TAG, "found %d tarot images", file_count);
  } else {
    ESP_LOGW(TAG, "no images under %s", TAROT_DIR);
  }
}

void Tarot_ShowRandom(void) {
  if (file_count == 0) {
    bool ready = sd_ready();
    if (!ready) {
      show_message("No SD card");
      return;
    }
    if (!scan_files()) {
      show_message("No images found");
      return;
    }
  }

  int slots = file_count < TAROT_CARD_SLOTS ? file_count : TAROT_CARD_SLOTS;

  /* Distinct indices for this draw. */
  int chosen[TAROT_CARD_SLOTS];
  for (int i = 0; i < slots; i++) {
    int index;
    bool duplicate;
    do {
      index = (int)(esp_random() % (uint32_t)file_count);
      duplicate = false;
      for (int j = 0; j < i; j++) {
        if (chosen[j] == index) {
          duplicate = true;
        }
      }
    } while (duplicate);
    chosen[i] = index;
  }

  lv_color_t* fresh[TAROT_CARD_SLOTS] = {};
  for (int i = 0; i < slots; i++) {
    char path[64];
    snprintf(path, sizeof(path), TAROT_DIR "/%s", file_names[chosen[i]]);

    uint8_t* rgb = nullptr;
    uint16_t width = 0;
    uint16_t height = 0;
    if (!decode_rgb565(path, &rgb, &width, &height)) {
      for (int j = 0; j < i; j++) {
        heap_caps_free(fresh[j]);
      }
      show_message("Card read failed");
      return;
    }
    fresh[i] = card_to_mono(rgb, width, height);
    heap_caps_free(rgb);
    if (fresh[i] == nullptr) {
      for (int j = 0; j <= i; j++) {
        heap_caps_free(fresh[j]);
      }
      show_message("Card read failed");
      return;
    }
  }

  /* Swap every slot under one lock so the LVGL task never renders a buffer
   * after it is freed, nor a descriptor before its pixels are set. */
  if (!Lvgl_lock(-1)) {
    for (int i = 0; i < slots; i++) {
      heap_caps_free(fresh[i]);
    }
    return;
  }
  for (int i = 0; i < TAROT_CARD_SLOTS; i++) {
    lv_color_t* previous = card_pixels[i];
    lv_color_t* next = (i < slots) ? fresh[i] : nullptr;
    card_pixels[i] = next;
    if (next == nullptr) {
      tarot_page_set_card(i, nullptr, nullptr);
    } else {
      card_dsc[i].header.always_zero = 0;
      card_dsc[i].header.reserved = 0;
      card_dsc[i].header.cf = LV_IMG_CF_TRUE_COLOR;
      card_dsc[i].header.w = TAROT_CARD_WIDTH;
      card_dsc[i].header.h = TAROT_CARD_HEIGHT;
      card_dsc[i].data_size = TAROT_CARD_WIDTH * TAROT_CARD_HEIGHT * sizeof(lv_color_t);
      card_dsc[i].data = (const uint8_t*)next;
      tarot_page_set_card(i, &card_dsc[i], file_labels[chosen[i]]);
    }
    if (previous != nullptr) {
      heap_caps_free(previous);
    }
  }
  Lvgl_unlock();
  Lvgl_RequestRender(20);

  ESP_LOGI(TAG, "drew %d cards", slots);
}
