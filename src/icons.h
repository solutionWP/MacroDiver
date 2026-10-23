#pragma once
#include <stdint.h>
#include <stddef.h>
#include <lvgl.h>

// Stratagem icons: 64 px large / 44 px small, LVGL TRUE_COLOR_ALPHA (RGB565 big-endian + A8).
// Built-in icons are linked into the firmware (data/icons.bin, tools/icongen). Icons added
// from the phone editor are files in the "spiffs" flash partition (LittleFS, /icons/<id>.bin),
// loaded into PSRAM at boot: for a custom stratagem, or replacing a built-in icon.
// Each icon remembers the git blob SHA of the library SVG it came from
// (github.com/nvigneux/Helldivers-2-Stratagems-icons-svg), so the editor can spot updates.

#define ICON_BYTES_LARGE (64 * 64 * 3)
#define ICON_BYTES_SMALL (44 * 44 * 3)
#define ICON_BYTES (ICON_BYTES_LARGE + ICON_BYTES_SMALL)  // upload format: large then small

void icons_begin();  // mount the partition, load stored icons

const lv_img_dsc_t *icon_get(uint16_t id, bool large);  // nullptr if none

// Library path / SHA of a stratagem's icon, "" if unknown
const char *icon_path(uint16_t id);  // built-ins only (custom ones keep theirs in CustomStrat)
const char *icon_sha(uint16_t id);

// Store an icon from the editor (ICON_BYTES); replaces any previous one. false if full or failed.
bool icon_store(uint16_t id, const char *sha, const uint8_t *data, size_t len);
void icon_remove(uint16_t id);   // stored icon only; a built-in one shows again
void icons_wipe();               // factory reset
uint32_t icons_version();        // changes whenever stored icons change (cache key for /icons)

// Web editor: small icons streamed in index order
int icon_count();
int icon_index(uint16_t id);  // -1 if none
int icon_small_size();        // pixels per side
const uint8_t *icon_small_data(int index);
