// Text backup of settings, builds, custom stratagems and chat messages.
// Format (one record per line, fields separated by '|'):
//   MACRODIVER-BACKUP 1
//   S menuKey=0 menuMode=0 ...            settings, unknown keys ignored
//   B layout|name|id,id,id                 one per build, in order
//   C id|name|code|cat|src                 custom stratagem (src: icon library path; the
//                                          icons themselves aren't in the backup: the phone
//                                          editor offers to download missing ones again)
//   M id|text                              chat message
//   END
// IDs are the stable catalog/custom/message IDs, so builds restore exactly.
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>
#include "storage.h"
#include "icons.h"

struct SetField { const char *key; uint8_t *ptr; };
static const SetField SET_FIELDS[] = {
  {"menuKey", &g_settings.menuKey}, {"menuMode", &g_settings.menuMode}, {"dirSet", &g_settings.dirSet},
  {"speed", &g_settings.speed}, {"brightness", &g_settings.brightness}, {"showCodes", &g_settings.showCodes},
  {"usbSerial", &g_settings.usbSerial}, {"activeBuild", &g_settings.activeBuild}, {"rotate180", &g_settings.rotate180},
  {"chatKey", &g_settings.chatKey}, {"textLayout", &g_settings.textLayout}, {"wifiOn", &g_settings.wifiOn},
  {"output", &g_settings.output},
};

static void put_clean(Stream *o, const char *s) {  // '|' and newlines would break the format
  for (; *s; s++) o->write((*s == '|' || *s == '\n' || *s == '\r') ? '/' : *s);
}

void backup_export(Stream *o) {
  o->println("MACRODIVER-BACKUP 1");
  o->print("S");
  for (auto &f : SET_FIELDS) o->printf(" %s=%u", f.key, *f.ptr);
  o->println();
  for (int i = 0; i < g_buildCount; i++) {
    const Build &b = g_builds[i];
    o->printf("B %u|", b.layout);
    put_clean(o, b.name);
    o->print("|");
    for (int k = 0; k < b.count; k++) o->printf(k ? ",%u" : "%u", b.ids[k]);
    o->println();
  }
  for (int j = 0; j < g_customCount; j++) {
    o->printf("C %u|", g_customs[j].id);
    put_clean(o, g_customs[j].name);
    o->printf("|%s|%u|", g_customs[j].code, g_customs[j].cat);
    put_clean(o, g_customs[j].src);
    o->println();
  }
  for (int j = 0; j < g_msgCount; j++) {
    o->printf("M %u|", g_msgs[j].id);
    put_clean(o, g_msgs[j].text);
    o->println();
  }
  o->println("END");
}

// Splits "a|b|c" in place; returns number of fields
static int split(char *s, char **f, int max) {
  int n = 0;
  f[n++] = s;
  for (char *p = s; *p && n < max; p++)
    if (*p == '|') { *p = 0; f[n++] = p + 1; }
  return n;
}

static bool s_importing = false;

// Feed one line. Returns 0 = not an import line, 1 = consumed, 2 = import complete (saved), -1 = error
int backup_import_line(char *line) {
  if (!strncmp(line, "MACRODIVER-BACKUP ", 18)) {
    s_importing = true;
    g_buildCount = 0;
    g_customCount = 0;
    g_msgCount = 0;
    return 1;
  }
  if (!s_importing) return 0;
  int n;
  char *f[5];
  if (line[0] == 'S' && line[1] == ' ') {
    for (char *tok = strtok(line + 2, " "); tok; tok = strtok(nullptr, " ")) {
      char *eq = strchr(tok, '=');
      if (!eq) continue;
      *eq = 0;
      for (auto &sf : SET_FIELDS)
        if (!strcmp(sf.key, tok)) *sf.ptr = (uint8_t)atoi(eq + 1);
    }
  } else if (line[0] == 'B' && line[1] == ' ' && split(line + 2, f, 3) == 3) {
    int i = build_add(f[1]);
    if (i < 0) return 1;
    Build &b = g_builds[i];
    b.layout = (uint8_t)atoi(f[0]);
    for (char *tok = strtok(f[2], ","); tok && b.count < MAX_SLOTS; tok = strtok(nullptr, ","))
      b.ids[b.count++] = (uint16_t)atoi(tok);
  } else if (line[0] == 'C' && line[1] == ' ' && (n = split(line + 2, f, 5)) >= 3) {
    // id|name|code, then |cat|src since the icon import (older backups: custom category, no icon)
    int j = custom_add(f[1], f[2], n >= 4 ? (uint8_t)atoi(f[3]) : CAT_CUSTOM, n >= 5 ? f[4] : "");
    if (j >= 0) {
      g_customs[j].id = (uint16_t)atoi(f[0]);  // keep original ID so builds still match
      custom_refresh(j);
    }
  } else if (line[0] == 'M' && line[1] == ' ' && split(line + 2, f, 2) == 2) {
    int j = msg_add(f[1]);
    if (j >= 0) {
      g_msgs[j].id = (uint16_t)atoi(f[0]);
      msg_refresh(j);
    }
  } else if (!strcmp(line, "END")) {
    s_importing = false;
    if (g_buildCount == 0) return -1;  // never save an empty set
    if (g_settings.activeBuild >= g_buildCount) g_settings.activeBuild = 0;
    storage_save_settings();
    storage_save_all_builds();
    storage_save_customs();
    storage_save_msgs();
    // icons of custom stratagems the backup doesn't have: a new one could reuse the ID
    for (uint16_t id = CUSTOM_ID_BASE; id < MSG_ID_BASE; id++)
      if (custom_index(id) < 0) icon_remove(id);
    return 2;
  }
  return 1;
}
