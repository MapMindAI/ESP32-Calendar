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
#include "tarot_page.h"

/* Cards are 350 x 600, exactly twice the 175 x 300 target, so the downscale is
 * a plain 2 x 2 box average. The result is built at its final size and drawn
 * 1:1, centred — no LVGL zoom or rotation at render time. */
#define TAROT_DIR "/sdcard/tarot/images"
#define TAROT_MAX_FILES 96
#define TAROT_NAME_LEN 20
#define TAROT_TARGET_W 175
#define TAROT_TARGET_H 300

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
static int file_count = 0;
static int last_index = -1;

/* The descriptor handed to LVGL is stable; its pixel buffer is swapped when a
 * new card is decoded. */
static lv_img_dsc_t card_dsc;
static lv_color_t* card_pixels = nullptr;

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

/* 2 x 2 box average plus ordered dither, straight into black/white RGB565.
 * Pure black and white survive the flush threshold unchanged, so the panel
 * shows exactly the dot pattern chosen here. */
static lv_color_t* card_to_mono(const uint8_t* rgb, uint16_t width, uint16_t height) {
  lv_color_t* pixels = (lv_color_t*)heap_caps_malloc(
      TAROT_TARGET_W * TAROT_TARGET_H * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
  if (pixels == nullptr) {
    ESP_LOGE(TAG, "no PSRAM for the mono frame");
    return nullptr;
  }

  const uint16_t* src = (const uint16_t*)rgb;
  for (int y = 0; y < TAROT_TARGET_H; y++) {
    for (int x = 0; x < TAROT_TARGET_W; x++) {
      uint32_t sum = 0;
      int samples = 0;
      for (int dy = 0; dy < 2; dy++) {
        for (int dx = 0; dx < 2; dx++) {
          int sx = x * 2 + dx;
          int sy = y * 2 + dy;
          if (sx >= width || sy >= height) {
            continue;
          }
          sum += pixel_luma(src[sy * width + sx]);
          samples++;
        }
      }
      int luma = samples ? (int)(sum / (uint32_t)samples) : 255;
      int threshold = (bayer4[y & 3][x & 3] + 1) * 16; /* 16 .. 256 */
      pixels[y * TAROT_TARGET_W + x] =
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

  int index = (int)(esp_random() % (uint32_t)file_count);
  if (file_count > 1 && index == last_index) {
    index = (index + 1) % file_count;
  }

  char path[64];
  snprintf(path, sizeof(path), TAROT_DIR "/%s", file_names[index]);

  uint8_t* rgb = nullptr;
  uint16_t width = 0;
  uint16_t height = 0;
  if (!decode_rgb565(path, &rgb, &width, &height)) {
    show_message("Card read failed");
    return;
  }
  lv_color_t* pixels = card_to_mono(rgb, width, height);
  heap_caps_free(rgb);
  if (pixels == nullptr) {
    show_message("Card read failed");
    return;
  }
  last_index = index;

  /* Swap under the lock so the LVGL task never sees the old buffer after it is
   * freed or a descriptor pointing at the new one before it is set. */
  if (!Lvgl_lock(-1)) {
    heap_caps_free(pixels);
    return;
  }
  lv_color_t* previous = card_pixels;
  card_pixels = pixels;
  card_dsc.header.always_zero = 0;
  card_dsc.header.reserved = 0;
  card_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
  card_dsc.header.w = TAROT_TARGET_W;
  card_dsc.header.h = TAROT_TARGET_H;
  card_dsc.data_size = TAROT_TARGET_W * TAROT_TARGET_H * sizeof(lv_color_t);
  card_dsc.data = (const uint8_t*)card_pixels;
  tarot_page_set_image(&card_dsc);
  if (previous != nullptr) {
    heap_caps_free(previous);
  }
  Lvgl_unlock();
  Lvgl_RequestRender(20);

  ESP_LOGI(TAG, "%s", path);
}
