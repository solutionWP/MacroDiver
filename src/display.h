#pragma once
#include <stdint.h>

#define SCREEN_W 412
#define SCREEN_H 412

void display_begin();          // I2C, IO expander, LCD, touch, LVGL
void display_set_brightness(uint8_t pct);
void display_set_rotation(bool rot180);  // screen + touch upside down
void display_loop();           // call often from loop()
void display_inject_touch(bool down, uint16_t x, uint16_t y);  // debug: simulated finger
