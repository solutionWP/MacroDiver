#pragma once
#include <stdint.h>
#include <stddef.h>
#include "macrolink.h"

// Option tables (indices stored in Settings)
struct KeyOption { const char *label; uint8_t key; };
struct DirSet { const char *label; uint8_t up, down, left, right; };
struct SpeedOption { const char *label; uint16_t openMs, pressMs, gapMs; };

extern const KeyOption MENU_KEYS[];
extern const size_t MENU_KEY_COUNT;
extern const DirSet DIR_SETS[];
extern const size_t DIR_SET_COUNT;
extern const SpeedOption SPEEDS[];
extern const size_t SPEED_COUNT;
extern const KeyOption CHAT_KEYS[];  // key = 0 -> chat already open, just type
extern const size_t CHAT_KEY_COUNT;
// Text layouts: MACRO_LAYOUTS / MACRO_LAYOUT_COUNT (macrolink.h)

// Keystrokes go out on this pad's USB cable or, with Settings -> Output:
// Wireless, through the dongle (link.h). These calls cover both.
void hid_begin(bool withSerial);
bool hid_connected();             // the selected output can type right now
const char *hid_output_name();    // "USB" / "Dongle", for messages
bool hid_send(const char *code);  // queue a U/D/L/R sequence; false if busy / not connected
bool hid_send_text(const char *text);  // open chat, type, Enter
bool hid_busy();
int hid_step();                   // index of the arrow currently being sent, -1 when idle
void hid_log(const char *fmt, ...);
class Stream;
Stream *hid_serial();             // USB CDC port, nullptr when disabled
