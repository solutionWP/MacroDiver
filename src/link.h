#pragma once
#include <stdint.h>
#include "macrolink.h"

// Wireless output: ESP-NOW link to the MacroDiver dongle (dongle/), which plugs
// into the PS5 adapter or PC as a USB keyboard and types the jobs we send.
// Shares the radio with the Wi-Fi editor (web.h): the dongle follows whatever
// channel the pad is on.
void link_begin();               // at boot: starts if Settings -> Output is Wireless
void link_set_enabled(bool on);  // saves the setting
bool link_enabled();
void link_loop();                // call from loop()
void link_wifi_changed();        // web.cpp: after Wi-Fi was turned off

bool link_connected();           // dongle heard recently and plugged into a host
bool link_dongle_seen();         // dongle heard recently (maybe without a host)
bool link_paired();
bool link_send(const MacroJob &job);  // false if not connected or busy
bool link_busy();
int link_step();

void link_pair_start();          // accept a new dongle for LINK_PAIR_MS
void link_pair_cancel();
int link_pair_seconds_left();    // 0 when not pairing
