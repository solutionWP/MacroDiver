// Serial debug commands on the USB CDC port (only when "USB serial" is enabled):
//   status | shot | tap x y | hold x y ms | swipe x1 y1 x2 y2 | boot | say <text>
//   export | MACRODIVER-BACKUP ... END (import, see backup.cpp) | webstate | websave <json>
// 'shot' replies "SHOT <w> <h> <bytes>\n" + raw RGB565 (big-endian) + "\nEND\n".
#include <Arduino.h>
#include <lvgl.h>
#include "display.h"
#include "storage.h"
#include "hid_out.h"
#include "ui.h"
#include "web.h"
#include "link.h"
#include <WiFi.h>

static void pump(uint32_t ms) {
  uint32_t end = millis() + ms;
  while ((int32_t)(end - millis()) > 0) {
    lv_timer_handler();
    delay(5);
  }
}

static void press_path(int x1, int y1, int x2, int y2, uint32_t ms) {
  const int steps = 12;
  display_inject_touch(true, x1, y1);
  pump(60);
  for (int i = 1; i <= steps; i++) {
    display_inject_touch(true, x1 + (x2 - x1) * i / steps, y1 + (y2 - y1) * i / steps);
    pump(ms / steps);
  }
  display_inject_touch(false, x2, y2);
  pump(120);
}

static void screenshot(Stream *out) {
  lv_img_dsc_t *snap = lv_snapshot_take(lv_scr_act(), LV_IMG_CF_TRUE_COLOR);
  if (!snap) {
    out->println("ERR snapshot");
    return;
  }
  size_t size = (size_t)snap->header.w * snap->header.h * sizeof(lv_color_t);  // data_size isn't filled in 8.3
  out->printf("SHOT %d %d %u\n", snap->header.w, snap->header.h, (unsigned)size);
  const uint8_t *p = snap->data;
  size_t left = size;
  while (left) {
    size_t n = left > 1024 ? 1024 : left;
    n = out->write(p, n);
    p += n;
    left -= n;
  }
  out->print("\nEND\n");
  lv_snapshot_free(snap);
}

void backup_export(Stream *o);         // backup.cpp
int backup_import_line(char *line);

void debug_poll() {
  Stream *s = hid_serial();
  if (!s || !s->available()) return;
  static char line[1024];
  static size_t n = 0;
  while (s->available()) {
    char c = s->read();
    if (c == '\r') continue;
    if (c != '\n') {
      if (n < sizeof(line) - 1) line[n++] = c;
      continue;
    }
    line[n] = 0;
    n = 0;
    int a, b, c2, d;
    int imp = backup_import_line(line);
    if (imp == 1) {
      s->println("OK");
    } else if (imp == 2) {
      s->println("OK imported, restarting");
      s->flush();
      delay(300);
      ESP.restart();
    } else if (imp < 0) {
      s->println("ERR backup has no builds, nothing saved");
    } else if (!strcmp(line, "webstate")) {
      s->println(web_state_json());
    } else if (!strncmp(line, "websave ", 8)) {
      const char *err = web_apply_json(line + 8);
      s->println(err ? err : "OK saved");
    } else if (!strcmp(line, "export")) {
      backup_export(s);
    } else if (!strcmp(line, "status")) {
      s->printf("OK usb=%d build='%s' items=%d layout=%d heap=%u psram=%u wifi=%d net=%d ip=%s home='%s' ap=%d "
                "out=%d paired=%d dongle=%d pairing=%d ch=%d\n",
                hid_connected(), build_active()->name, build_active()->count, build_active()->layout,
                (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getFreePsram(), g_settings.wifiOn, web_state(), web_ip(),
                web_home_ssid(), web_ap_on(), g_settings.output, link_paired(), link_dongle_seen(),
                link_pair_seconds_left(), WiFi.getMode() == WIFI_OFF ? 0 : WiFi.channel());
    } else if (!strcmp(line, "shot")) {
      screenshot(s);
    } else if (sscanf(line, "tap %d %d", &a, &b) == 2) {
      press_path(a, b, a, b, 80);
      s->println("OK");
    } else if (sscanf(line, "hold %d %d %d", &a, &b, &c2) == 3) {
      press_path(a, b, a, b, c2);
      s->println("OK");
    } else if (sscanf(line, "swipe %d %d %d %d", &a, &b, &c2, &d) == 4) {
      press_path(a, b, c2, d, 300);
      s->println("OK");
    } else if (!strncmp(line, "say ", 4)) {
      s->println(hid_send_text(line + 4) ? "OK" : "ERR busy");
    } else if (!strcmp(line, "boot")) {
      ui_boot_button();
      pump(100);
      s->println("OK");
    } else if (line[0]) {
      s->println("ERR unknown");
    }
  }
}
