#include "display.h"
#include <Arduino.h>
#include <lvgl.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include "board/I2C_Driver.h"
#include "board/TCA9554PWR.h"
#include "board/Display_SPD2010.h"
#include "board/Touch_SPD2010.h"

#define BUF_LINES 52  // 412 x 52 px per buffer, 2 buffers in internal DMA RAM
#define TOUCH_RELEASE_MS 60

static lv_disp_draw_buf_t s_drawBuf;

// SPD2010 needs x-start aligned to 4 and width a multiple of 4
static void rounder_cb(lv_disp_drv_t *, lv_area_t *area) {
  area->x1 = (area->x1 >> 2) << 2;
  area->x2 = ((area->x2 >> 2) << 2) + 3;
}

static volatile bool s_rot180 = false;

// 180 deg rotation in software: reversing the pixel order of a rectangle rotates
// it, and the rectangle moves to the opposite corner. x stays 4-aligned since
// the rounder makes x1 = 4n and x2 = 4m+3 (411 - x2 = 4(102-m)).
static void flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color) {
  if (s_rot180) {
    uint16_t *px = (uint16_t *)&color->full;
    size_t n = (size_t)(area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);
    for (size_t i = 0, j = n - 1; i < j; i++, j--) {
      uint16_t t = px[i]; px[i] = px[j]; px[j] = t;
    }
    LCD_addWindow(SCREEN_W - 1 - area->x2, SCREEN_H - 1 - area->y2, SCREEN_W - 1 - area->x1, SCREEN_H - 1 - area->y1, px);
  } else {
    LCD_addWindow(area->x1, area->y1, area->x2, area->y2, (uint16_t *)&color->full);
  }
  lv_disp_flush_ready(drv);
}

void display_set_rotation(bool rot180) {
  s_rot180 = rot180;
  lv_obj_invalidate(lv_scr_act());
}

// Simulated touch (serial debug commands)
static volatile bool s_injDown = false;
static volatile uint16_t s_injX = 0, s_injY = 0;

void display_inject_touch(bool down, uint16_t x, uint16_t y) {
  s_injX = x;
  s_injY = y;
  s_injDown = down;
}

// The SPD2010 doesn't report on every poll while a finger rests, so a single
// empty read is not a release. Only release after TOUCH_RELEASE_MS of silence
// or an explicit zero-weight report. Avoids phantom double taps.
static void touch_cb(lv_indev_drv_t *, lv_indev_data_t *data) {
  static bool down = false;
  static uint16_t lx = 0, ly = 0;
  static uint32_t lastSeen = 0;
  uint16_t x[CONFIG_ESP_LCD_TOUCH_MAX_POINTS], y[CONFIG_ESP_LCD_TOUCH_MAX_POINTS], w[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
  uint8_t cnt = 0;
  uint32_t now = millis();

  if (s_injDown) {
    data->point.x = s_injX;
    data->point.y = s_injY;
    data->state = LV_INDEV_STATE_PR;
    return;
  }
  if (Touch_Get_xy(x, y, w, &cnt, CONFIG_ESP_LCD_TOUCH_MAX_POINTS) && cnt > 0) {
    if (w[0] == 0) {
      down = false;
    } else {
      down = true;
      lx = s_rot180 ? SCREEN_W - 1 - x[0] : x[0];
      ly = s_rot180 ? SCREEN_H - 1 - y[0] : y[0];
      lastSeen = now;
    }
  } else if (down && now - lastSeen > TOUCH_RELEASE_MS) {
    down = false;
  }
  data->point.x = lx;
  data->point.y = ly;
  data->state = down ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
}

static void tick_cb(void *) { lv_tick_inc(2); }

void display_set_brightness(uint8_t pct) {
  if (pct < 5) pct = 5;
  if (pct > 100) pct = 100;
  Set_Backlight(pct);
}

void display_begin() {
  I2C_Init();
  TCA9554PWR_Init(0x00);  // all expander pins outputs (LCD / touch reset)
  Backlight_Init();
  Set_Backlight(0);
  LCD_Init();             // SPD2010 panel + touch

  lv_init();
  size_t bufPx = SCREEN_W * BUF_LINES;
  lv_color_t *b1 = (lv_color_t *)heap_caps_malloc(bufPx * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  lv_color_t *b2 = (lv_color_t *)heap_caps_malloc(bufPx * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  lv_disp_draw_buf_init(&s_drawBuf, b1, b2, bufPx);

  static lv_disp_drv_t disp;
  lv_disp_drv_init(&disp);
  disp.hor_res = SCREEN_W;
  disp.ver_res = SCREEN_H;
  disp.flush_cb = flush_cb;
  disp.rounder_cb = rounder_cb;
  disp.full_refresh = 1;  // as in Waveshare's reference driver for this panel
  disp.draw_buf = &s_drawBuf;
  lv_disp_drv_register(&disp);

  static lv_indev_drv_t indev;
  lv_indev_drv_init(&indev);
  indev.type = LV_INDEV_TYPE_POINTER;
  indev.read_cb = touch_cb;
  indev.scroll_limit = 14;  // px before a press turns into a swipe
  indev.long_press_time = 500;
  lv_indev_drv_register(&indev);

  const esp_timer_create_args_t args = {.callback = tick_cb, .name = "lv_tick"};
  esp_timer_handle_t t;
  esp_timer_create(&args, &t);
  esp_timer_start_periodic(t, 2000);
}

void display_loop() { lv_timer_handler(); }
