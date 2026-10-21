#include "storage.h"
#include <Preferences.h>
#include <string.h>
#include <stdio.h>

#define STORE_VERSION 1

Settings g_settings;
Build g_builds[MAX_BUILDS];
int g_buildCount = 0;
CustomStrat g_customs[MAX_CUSTOM];
int g_customCount = 0;
ChatMsg g_msgs[MAX_MSG];
int g_msgCount = 0;

// Strat views of the custom entries so the UI can treat them like catalog items
static Strat s_customStrat[MAX_CUSTOM];
static Strat s_msgStrat[MAX_MSG];

static const char *DEFAULT_MSGS = "gg;have to go, see ya;see ya;bunker";

static Preferences prefs;

static void defaults_settings() {
  g_settings.menuKey = 0;     // Left Ctrl
  g_settings.menuMode = 0;    // hold
  g_settings.dirSet = 0;      // WASD
  g_settings.speed = 1;       // normal
  g_settings.brightness = 80;
  g_settings.showCodes = 1;
  g_settings.usbSerial = 1;
  g_settings.activeBuild = 0;
  g_settings.rotate180 = 0;
  g_settings.chatKey = 0;
  g_settings.textLayout = 0;
  g_settings.output = 0;      // USB cable
}

static void defaults_builds() {
  g_buildCount = 0;
  int i = build_add("Starter");
  Build &b = g_builds[i];
  const uint16_t ids[] = {51, 104, 26, 10, 200, 201, 203, 202};  // Quasar, Rover, 500kg, Laser, Reinforce, Resupply, Hellbomb, SoS
  b.count = sizeof(ids) / sizeof(ids[0]);
  memcpy(b.ids, ids, sizeof(ids));
  b.layout = LAYOUT_LARGE;
}

void custom_refresh(int j) {
  Strat &s = s_customStrat[j];
  s.id = g_customs[j].id;
  s.cat = CAT_CUSTOM;
  s.name = g_customs[j].name;
  s.shortName = g_customs[j].name;
  s.code = g_customs[j].code;
}

void msg_refresh(int j) {
  Strat &s = s_msgStrat[j];
  s.id = g_msgs[j].id;
  s.cat = CAT_MESSAGE;
  s.name = g_msgs[j].text;
  s.shortName = g_msgs[j].text;
  s.code = "";
}

static void defaults_msgs() {
  g_msgCount = 0;
  char buf[200];
  strncpy(buf, DEFAULT_MSGS, sizeof(buf));
  for (char *t = strtok(buf, ";"); t; t = strtok(nullptr, ";")) {
    while (*t == ' ') t++;
    if (*t) msg_add(t);
  }
}

void storage_load() {
  prefs.begin("macrodiver", false);
  bool ok = prefs.getUChar("ver", 0) == STORE_VERSION;

  // Settings may be an older, shorter blob: keep what's there, zero the new fields
  size_t setLen = ok ? prefs.getBytesLength("set") : 0;
  if (setLen == 0 || setLen > sizeof(g_settings)) {
    defaults_settings();
  } else {
    memset(&g_settings, 0, sizeof(g_settings));
    prefs.getBytes("set", &g_settings, setLen);
  }

  g_buildCount = 0;
  int nb = ok ? prefs.getUChar("nb", 0) : 0;
  char key[8];
  for (int i = 0; i < nb && i < MAX_BUILDS; i++) {
    snprintf(key, sizeof(key), "b%d", i);
    if (prefs.getBytes(key, &g_builds[g_buildCount], sizeof(Build)) == sizeof(Build)) {
      Build &b = g_builds[g_buildCount];
      b.name[NAME_LEN] = 0;
      if (b.count > MAX_SLOTS) b.count = MAX_SLOTS;
      g_buildCount++;
    }
  }

  g_customCount = 0;
  if (ok) {
    int nc = prefs.getUChar("nc", 0);
    if (nc > MAX_CUSTOM) nc = MAX_CUSTOM;
    if (nc > 0 && prefs.getBytes("c", g_customs, nc * sizeof(CustomStrat)) == nc * sizeof(CustomStrat))
      g_customCount = nc;
  }
  for (int j = 0; j < g_customCount; j++) custom_refresh(j);

  if (ok && prefs.isKey("nm")) {
    int nm = prefs.getUChar("nm", 0);
    if (nm > MAX_MSG) nm = MAX_MSG;
    g_msgCount = 0;
    if (nm > 0 && prefs.getBytes("m", g_msgs, nm * sizeof(ChatMsg)) == nm * sizeof(ChatMsg)) g_msgCount = nm;
    for (int j = 0; j < g_msgCount; j++) msg_refresh(j);
  } else {
    defaults_msgs();
    storage_save_msgs();
  }

  if (g_buildCount == 0) {
    defaults_builds();
    prefs.putUChar("ver", STORE_VERSION);
    storage_save_settings();
    storage_save_all_builds();
    storage_save_customs();
  }
  if (g_settings.activeBuild >= g_buildCount) g_settings.activeBuild = 0;
}

void storage_save_settings() { prefs.putBytes("set", &g_settings, sizeof(g_settings)); }

void storage_save_build(int i) {
  char key[8];
  snprintf(key, sizeof(key), "b%d", i);
  prefs.putBytes(key, &g_builds[i], sizeof(Build));
}

void storage_save_all_builds() {
  char key[8];
  for (int i = 0; i < g_buildCount; i++) storage_save_build(i);
  for (int i = g_buildCount; i < MAX_BUILDS; i++) {
    snprintf(key, sizeof(key), "b%d", i);
    if (prefs.isKey(key)) prefs.remove(key);
  }
  prefs.putUChar("nb", g_buildCount);
}

void storage_save_customs() {
  prefs.putUChar("nc", g_customCount);
  if (g_customCount) prefs.putBytes("c", g_customs, g_customCount * sizeof(CustomStrat));
  else if (prefs.isKey("c")) prefs.remove("c");
}

void storage_factory_reset() { prefs.clear(); }

void storage_get_str(const char *key, char *out, size_t len) {
  out[0] = 0;
  if (prefs.isKey(key)) prefs.getString(key, out, len);
}

void storage_put_str(const char *key, const char *val) {
  if (val && val[0]) prefs.putString(key, val);
  else if (prefs.isKey(key)) prefs.remove(key);
}

Build *build_active() { return &g_builds[g_settings.activeBuild]; }

int build_add(const char *name) {
  if (g_buildCount >= MAX_BUILDS) return -1;
  Build &b = g_builds[g_buildCount];
  memset(&b, 0, sizeof(b));
  strncpy(b.name, name, NAME_LEN);
  b.layout = LAYOUT_LARGE;
  return g_buildCount++;
}

int build_duplicate(int i) {
  if (g_buildCount >= MAX_BUILDS) return -1;
  Build &b = g_builds[g_buildCount];
  b = g_builds[i];
  char name[NAME_LEN + 1];
  snprintf(name, sizeof(name), "%.13s copy", g_builds[i].name);
  strncpy(b.name, name, NAME_LEN);
  b.name[NAME_LEN] = 0;
  return g_buildCount++;
}

void build_delete(int i) {
  if (g_buildCount <= 1) return;  // always keep one build
  for (int k = i; k < g_buildCount - 1; k++) g_builds[k] = g_builds[k + 1];
  g_buildCount--;
  if (g_settings.activeBuild == i) g_settings.activeBuild = 0;
  else if (g_settings.activeBuild > i) g_settings.activeBuild--;
}

int build_find_slot(const Build *b, uint16_t id) {
  for (int k = 0; k < b->count; k++)
    if (b->ids[k] == id) return k;
  return -1;
}

bool build_toggle(Build *b, uint16_t id) {
  int k = build_find_slot(b, id);
  if (k >= 0) {
    for (; k < b->count - 1; k++) b->ids[k] = b->ids[k + 1];
    b->count--;
    return false;
  }
  if (b->count >= MAX_SLOTS) return false;
  b->ids[b->count++] = id;
  return true;
}

int custom_add(const char *name, const char *code) {
  if (g_customCount >= MAX_CUSTOM) return -1;
  uint16_t id = CUSTOM_ID_BASE;
  for (bool clash = true; clash;) {  // lowest free ID
    clash = false;
    for (int j = 0; j < g_customCount; j++)
      if (g_customs[j].id == id) { id++; clash = true; }
  }
  CustomStrat &c = g_customs[g_customCount];
  memset(&c, 0, sizeof(c));
  c.id = id;
  strncpy(c.name, name, NAME_LEN);
  strncpy(c.code, code, MAX_CODE);
  custom_refresh(g_customCount);
  return g_customCount++;
}

void custom_delete(int j) {
  uint16_t id = g_customs[j].id;
  for (int i = 0; i < g_buildCount; i++) {
    if (build_find_slot(&g_builds[i], id) >= 0) {
      build_toggle(&g_builds[i], id);
      storage_save_build(i);
    }
  }
  for (int k = j; k < g_customCount - 1; k++) g_customs[k] = g_customs[k + 1];
  g_customCount--;
  for (int k = 0; k < g_customCount; k++) custom_refresh(k);
}

const Strat *custom_find(uint16_t id) {
  for (int j = 0; j < g_customCount; j++)
    if (g_customs[j].id == id) return &s_customStrat[j];
  return nullptr;
}

void storage_save_msgs() {
  prefs.putUChar("nm", g_msgCount);
  if (g_msgCount) prefs.putBytes("m", g_msgs, g_msgCount * sizeof(ChatMsg));
  else if (prefs.isKey("m")) prefs.remove("m");
}

int msg_add(const char *text) {
  if (g_msgCount >= MAX_MSG) return -1;
  uint16_t id = MSG_ID_BASE;
  for (bool clash = true; clash;) {  // lowest free ID
    clash = false;
    for (int j = 0; j < g_msgCount; j++)
      if (g_msgs[j].id == id) { id++; clash = true; }
  }
  ChatMsg &m = g_msgs[g_msgCount];
  memset(&m, 0, sizeof(m));
  m.id = id;
  strncpy(m.text, text, MSG_LEN);
  msg_refresh(g_msgCount);
  return g_msgCount++;
}

void msg_delete(int j) {
  uint16_t id = g_msgs[j].id;
  for (int i = 0; i < g_buildCount; i++) {
    if (build_find_slot(&g_builds[i], id) >= 0) {
      build_toggle(&g_builds[i], id);
      storage_save_build(i);
    }
  }
  for (int k = j; k < g_msgCount - 1; k++) g_msgs[k] = g_msgs[k + 1];
  g_msgCount--;
  for (int k = 0; k < g_msgCount; k++) msg_refresh(k);
}

const Strat *msg_find(uint16_t id) {
  for (int j = 0; j < g_msgCount; j++)
    if (g_msgs[j].id == id) return &s_msgStrat[j];
  return nullptr;
}
