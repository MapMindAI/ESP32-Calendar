#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <esp_log.h>
#include <esp_timer.h>
#include "lvgl_bsp.h"

static lv_disp_draw_buf_t disp_buf; 		// contains internal graphic buffer(s) called draw buffer(s)
static lv_disp_drv_t disp_drv;      		// contains callback functions
static SemaphoreHandle_t lvgl_mux = NULL;
static TaskHandle_t lvgl_task = NULL;

static const char *TAG = "LvglPort";

static void Increase_lvgl_tick(void* arg) { lv_tick_inc(LVGL_TICK_PERIOD_MS); }

bool Lvgl_lock(int timeout_ms) {
  const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
  return xSemaphoreTake(lvgl_mux, timeout_ticks) == pdTRUE;
}

void Lvgl_unlock(void) {
  assert(lvgl_mux && "bsp_display_start must be called first");
  xSemaphoreGive(lvgl_mux);
}

void Lvgl_RequestRender(int id) {
#if LVGL_DEBUG_LOG
  ESP_LOGI(TAG, "LVGL render request (%d)", id);
#endif
  assert(lvgl_task && "Lvgl_PortInit must be called first");
  xTaskNotifyGive(lvgl_task);
}

void Lvgl_RenderNow(int id) {
#if LVGL_DEBUG_LOG
  ESP_LOGI(TAG, "LVGL synchronous render request (%d)", id);
#endif
  assert(lvgl_task && "Lvgl_PortInit must be called first");
  if (Lvgl_lock(-1)) {
    lv_refr_now(NULL);
    Lvgl_unlock();
#if LVGL_DEBUG_LOG
    ESP_LOGI(TAG, "LVGL synchronous render complete");
#endif
  }
}

static void Lvgl_port_task(void* arg) {
  for (;;) {
    /* There are no LVGL animations or input devices on this display. Run a
     * refresh only when an app update or view switch requests it. */
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    if (Lvgl_lock(-1)) {
      /* Do not wait for LVGL's periodic refresh timer: a request means there
       * is an invalidated object that should reach the panel now. */
      lv_refr_now(NULL);
      // Release the mutex
      Lvgl_unlock();
#if LVGL_DEBUG_LOG
      ESP_LOGI(TAG, "LVGL render complete");
#endif
    }
  }
}

void Lvgl_PortInit(int width, int height, DispFlushCb flush_cb) {
  lvgl_mux = xSemaphoreCreateMutex();
  lv_init();
  lv_color_t* buffer1 =
      (lv_color_t*)heap_caps_malloc(width * height * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
  assert(buffer1);
  lv_color_t* buffer2 =
      (lv_color_t*)heap_caps_malloc(width * height * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
  assert(buffer2);

  lv_disp_draw_buf_init(&disp_buf, buffer1, buffer2, width * height);
  ESP_LOGI(TAG, "Register display driver to LVGL");

  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = width;
  disp_drv.ver_res = height;
  disp_drv.flush_cb = flush_cb;
  disp_drv.full_refresh = 1;
  disp_drv.draw_buf = &disp_buf;
  lv_disp_drv_register(&disp_drv);

  ESP_LOGI(TAG, "Install LVGL tick timer");
  esp_timer_create_args_t lvgl_tick_timer_args = {};
  lvgl_tick_timer_args.callback = &Increase_lvgl_tick;
  lvgl_tick_timer_args.name = "lvgl_tick";
  esp_timer_handle_t lvgl_tick_timer = NULL;
  ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

  xTaskCreatePinnedToCore(Lvgl_port_task, "LVGL", 8 * 1024, NULL, 5, &lvgl_task, 0);
  assert(lvgl_task);
}
