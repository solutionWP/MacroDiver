#pragma once
#include <stdint.h>
#include <lvgl.h>

// Stratagem icon (64 px large / 44 px small), or nullptr if none (e.g. custom).
const lv_img_dsc_t *icon_get(uint16_t id, bool large);

// Web editor: small icons streamed in index order (raw RGB565 big-endian + A8)
int icon_count();
int icon_index(uint16_t id);  // -1 if none
int icon_small_size();        // pixels per side
const uint8_t *icon_small_data(int index);
