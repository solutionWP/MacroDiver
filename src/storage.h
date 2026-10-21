#pragma once
#include <stdint.h>
#include <stddef.h>
#include "stratagems.h"

#define MAX_BUILDS  12
#define MAX_SLOTS   24
#define NAME_LEN    18
#define MAX_CUSTOM  16
#define MAX_CODE    10
#define MAX_MSG     12
#define MSG_LEN     40

enum Layout : uint8_t { LAYOUT_LARGE = 0, LAYOUT_DENSE = 1 };  // 4 or 7 tiles per page

struct Build {
  char name[NAME_LEN + 1];
  uint8_t layout;
  uint8_t count;
  uint16_t ids[MAX_SLOTS];
};

struct CustomStrat {
  uint16_t id;  // >= CUSTOM_ID_BASE, stable
  char name[NAME_LEN + 1];
  char code[MAX_CODE + 1];
};

struct ChatMsg {
  uint16_t id;  // >= MSG_ID_BASE, stable
  char text[MSG_LEN + 1];
};

// New fields go at the end: older saved blobs load with them zeroed.
struct Settings {
  uint8_t menuKey;     // index into hid menu key table
  uint8_t menuMode;    // 0 = hold, 1 = tap
  uint8_t dirSet;      // index into direction set table
  uint8_t speed;       // index into speed table
  uint8_t brightness;  // 10..100
  uint8_t showCodes;   // arrows on tiles
  uint8_t usbSerial;   // expose USB serial port (firmware updates); 0 = pure keyboard
  uint8_t activeBuild;
  uint8_t rotate180;   // screen upside down
  uint8_t chatKey;     // index into chat key table (0 = Enter)
  uint8_t textLayout;  // index into keyboard layout table for typed messages
  uint8_t wifiOn;      // Wi-Fi + web editor running (persists across reboots)
  uint8_t output;      // 0 = this pad's USB cable, 1 = wireless dongle (link.h)
};

extern Settings g_settings;
extern Build g_builds[MAX_BUILDS];
extern int g_buildCount;
extern CustomStrat g_customs[MAX_CUSTOM];
extern int g_customCount;
extern ChatMsg g_msgs[MAX_MSG];
extern int g_msgCount;

void storage_load();
void storage_save_settings();
void storage_save_build(int i);
void storage_save_all_builds();  // after insert/delete (indices shift)
void storage_save_customs();
void storage_save_msgs();
void storage_factory_reset();  // wipes everything (caller restarts)

// Small strings outside the main structs (Wi-Fi credentials, AP password)
void storage_get_str(const char *key, char *out, size_t len);  // "" if missing
void storage_put_str(const char *key, const char *val);        // empty value removes the key

Build *build_active();
int build_add(const char *name);        // returns index or -1
int build_duplicate(int i);
void build_delete(int i);
int build_find_slot(const Build *b, uint16_t id);
bool build_toggle(Build *b, uint16_t id);  // add at end / remove; returns true if now present

int custom_add(const char *name, const char *code);  // returns index or -1
void custom_delete(int j);                            // also removes it from builds
const Strat *custom_find(uint16_t id);
void custom_refresh(int j);                           // after editing name/code

int msg_add(const char *text);  // returns index or -1
void msg_delete(int j);         // also removes it from builds
const Strat *msg_find(uint16_t id);
void msg_refresh(int j);        // after editing text
