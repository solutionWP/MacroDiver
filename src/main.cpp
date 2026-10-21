// MacroDiver - Helldivers 2 stratagem macropad for the Waveshare ESP32-S3-Touch-LCD-1.46
//
// The device enumerates as a USB keyboard. Each tile sends:
//   [hold menu key] -> arrow keys of the stratagem code -> [release menu key]
// PC: works natively (default L-Ctrl + WASD).
// PS5: Helldivers 2 has no keyboard support on PS5, so plug the pad into a
//      keyboard-to-controller adapter (map menu key -> L1, WASD -> D-pad) or
//      use PS Remote Play / chiaki-ng on a PC. See README.md.
// Wireless: with Settings -> Output: Wireless, the keystrokes go over ESP-NOW
//      to the dongle (dongle/), which is the USB keyboard instead (link.h).
#include <Arduino.h>
#include "display.h"
#include "storage.h"
#include "hid_out.h"
#include "ui.h"
#include "web.h"
#include "link.h"

#define BOOT_BTN_PIN 0

void debug_poll();  // debug_cmd.cpp

void setup() {
  Serial.begin(115200);
  storage_load();
  hid_begin(g_settings.usbSerial);
  display_begin();
  display_set_rotation(g_settings.rotate180);
  ui_begin();
  web_begin();  // Wi-Fi + editor, if enabled in Settings
  link_begin();  // wireless dongle, if Settings -> Output is Wireless (after Wi-Fi: shares the radio)
  display_set_brightness(g_settings.brightness);
  pinMode(BOOT_BTN_PIN, INPUT_PULLUP);
  hid_log("MacroDiver ready: %d builds, %d custom, active '%s'", g_buildCount, g_customCount, build_active()->name);
}

void loop() {
  display_loop();

  // BOOT button: toggle pad <-> menu (debounced, on press)
  static bool lastDown = false;
  static uint32_t lastChange = 0;
  bool down = digitalRead(BOOT_BTN_PIN) == LOW;
  if (down != lastDown && millis() - lastChange > 40) {
    lastChange = millis();
    lastDown = down;
    if (down) ui_boot_button();
  }
  debug_poll();
  web_loop();
  link_loop();
  ui_poll();
  delay(4);
}
