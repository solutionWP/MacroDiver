#include "icons.h"
#include "icons_index.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <esp_random.h>

// data/icons.bin is linked into flash via board_build.embed_files
extern const uint8_t icons_bin_start[] asm("_binary_data_icons_bin_start");

#define MAX_STORED 160  // ~18 KB each: fits the 3.4 MB partition with room to spare
#define ICON_DIR "/icons"

struct __attribute__((packed)) IconFileHdr {  // file = header + ICON_BYTES
  char magic[4];  // "MDI1"
  uint16_t id;
  uint16_t reserved;
  char sha[40];
};

// A stored icon. Removing one only clears its id: the PSRAM buffer is reused by the next
// store, so an image LVGL still shows never points at freed memory.
struct Stored {
  uint16_t id;  // 0 = free slot
  char sha[41];
  uint8_t *data;
  lv_img_dsc_t dsc[2];  // large, small
};

static Stored s_st[MAX_STORED];
static int s_nst = 0;
static lv_img_dsc_t s_builtin[ICON_COUNT][2];
static bool s_ready = false, s_fs = false;
static uint32_t s_ver = 0;

static void make_dsc(lv_img_dsc_t &d, int sz, const uint8_t *px) {
  memset(&d, 0, sizeof(d));
  d.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
  d.header.w = sz;
  d.header.h = sz;
  d.data_size = sz * sz * LV_IMG_PX_SIZE_ALPHA_BYTE;
  d.data = px;
}

static int builtin_index(uint16_t id) {
  for (int i = 0; i < ICON_COUNT; i++)
    if (ICON_INDEX[i].id == id) return i;
  return -1;
}

static Stored *stored_find(uint16_t id) {
  for (int i = 0; i < s_nst; i++)
    if (s_st[i].id == id) return &s_st[i];
  return nullptr;
}

// Puts data into RAM (existing entry updated in place, else a free or new slot)
static bool stored_put(uint16_t id, const char *sha, const uint8_t *data) {
  Stored *e = stored_find(id);
  if (!e)
    for (int i = 0; i < s_nst && !e; i++)
      if (!s_st[i].id) e = &s_st[i];
  if (!e) {
    if (s_nst >= MAX_STORED) return false;
    uint8_t *buf = (uint8_t *)ps_malloc(ICON_BYTES);
    if (!buf) return false;
    e = &s_st[s_nst++];
    e->data = buf;
    make_dsc(e->dsc[0], 64, buf);
    make_dsc(e->dsc[1], 44, buf + ICON_BYTES_LARGE);
  }
  memcpy(e->data, data, ICON_BYTES);
  strncpy(e->sha, sha, 40);
  e->sha[40] = 0;
  e->id = id;
  return true;
}

static void file_name(uint16_t id, char *out, size_t n) { snprintf(out, n, ICON_DIR "/%u.bin", id); }

static void load_stored() {
  File dir = LittleFS.open(ICON_DIR);
  if (!dir || !dir.isDirectory()) {
    LittleFS.mkdir(ICON_DIR);
    return;
  }
  uint8_t *buf = (uint8_t *)ps_malloc(ICON_BYTES);
  if (!buf) return;
  for (File f = dir.openNextFile(); f; f = dir.openNextFile()) {
    IconFileHdr h;
    if (f.size() == sizeof(h) + ICON_BYTES && f.read((uint8_t *)&h, sizeof(h)) == sizeof(h) &&
        !memcmp(h.magic, "MDI1", 4) && h.id && f.read(buf, ICON_BYTES) == ICON_BYTES) {
      char sha[41];
      memcpy(sha, h.sha, 40);
      sha[40] = 0;
      stored_put(h.id, sha, buf);
    }
    f.close();
  }
  free(buf);
}

void icons_begin() {
  if (s_ready) return;
  for (int i = 0; i < ICON_COUNT; i++) {
    make_dsc(s_builtin[i][0], ICON_LARGE, icons_bin_start + ICON_INDEX[i].offLarge);
    make_dsc(s_builtin[i][1], ICON_SMALL, icons_bin_start + ICON_INDEX[i].offSmall);
  }
  s_ver = esp_random();
  s_ready = true;
  // formatOnFail: the partition is blank on boards flashed before this firmware
  s_fs = LittleFS.begin(true, "/littlefs", 10, "spiffs");
  if (s_fs) load_stored();
}

const lv_img_dsc_t *icon_get(uint16_t id, bool large) {
  if (!s_ready) icons_begin();
  if (Stored *e = stored_find(id)) return &e->dsc[large ? 0 : 1];
  int i = builtin_index(id);
  return i >= 0 ? &s_builtin[i][large ? 0 : 1] : nullptr;
}

const char *icon_path(uint16_t id) {
  int i = builtin_index(id);
  return i >= 0 ? ICON_INDEX[i].path : "";
}

const char *icon_sha(uint16_t id) {
  if (Stored *e = stored_find(id)) return e->sha;
  int i = builtin_index(id);
  return i >= 0 ? ICON_INDEX[i].sha : "";
}

bool icon_store(uint16_t id, const char *sha, const uint8_t *data, size_t len) {
  if (!s_fs || !id || len != ICON_BYTES) return false;
  char name[24];
  file_name(id, name, sizeof(name));
  if (!stored_find(id) && LittleFS.totalBytes() - LittleFS.usedBytes() < ICON_BYTES + 8192) return false;
  File f = LittleFS.open(name, "w");
  if (!f) return false;
  IconFileHdr h;
  memcpy(h.magic, "MDI1", 4);
  h.id = id;
  h.reserved = 0;
  memset(h.sha, 0, sizeof(h.sha));
  strncpy(h.sha, sha ? sha : "", sizeof(h.sha));
  bool ok = f.write((const uint8_t *)&h, sizeof(h)) == sizeof(h) && f.write(data, len) == len;
  f.close();
  if (!ok || !stored_put(id, h.sha[0] ? sha : "", data)) {
    LittleFS.remove(name);
    return false;
  }
  s_ver++;
  return true;
}

void icon_remove(uint16_t id) {
  Stored *e = stored_find(id);
  if (!e) return;
  e->id = 0;
  char name[24];
  file_name(id, name, sizeof(name));
  if (s_fs) LittleFS.remove(name);
  s_ver++;
}

void icons_wipe() {
  for (int i = 0; i < s_nst; i++) s_st[i].id = 0;
  if (s_fs) LittleFS.format();
  s_ver++;
}

uint32_t icons_version() { return s_ver; }

// Index space for the editor: built-ins first (a stored replacement wins), then stored
// icons of custom stratagems, in slot order.
static bool extra(const Stored &e) { return e.id && builtin_index(e.id) < 0; }

int icon_count() {
  int n = ICON_COUNT;
  for (int i = 0; i < s_nst; i++) n += extra(s_st[i]);
  return n;
}

int icon_small_size() { return ICON_SMALL; }

int icon_index(uint16_t id) {
  int i = builtin_index(id);
  if (i >= 0) return i;
  int n = ICON_COUNT;
  for (int k = 0; k < s_nst; k++) {
    if (!extra(s_st[k])) continue;
    if (s_st[k].id == id) return n;
    n++;
  }
  return -1;
}

const uint8_t *icon_small_data(int index) {
  if (index < ICON_COUNT) {
    if (Stored *e = stored_find(ICON_INDEX[index].id)) return e->data + ICON_BYTES_LARGE;
    return icons_bin_start + ICON_INDEX[index].offSmall;
  }
  int n = ICON_COUNT;
  for (int k = 0; k < s_nst; k++) {
    if (!extra(s_st[k])) continue;
    if (n == index) return s_st[k].data + ICON_BYTES_LARGE;
    n++;
  }
  return nullptr;
}
