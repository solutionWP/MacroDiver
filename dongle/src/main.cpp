// MacroDiver dongle: ESP32-S3 Super Mini that plugs into the PS5 keyboard
// adapter (or a PC) as a USB keyboard and types what the pad sends over ESP-NOW.
// Protocol and keystroke timing are shared with the pad: lib/macrolink.
//
// LED:  purple blink  not paired yet (pad: Settings -> Dongle -> tap to pair)
//       blue blink    looking for its pad
//       green         linked and plugged into a USB host
//       yellow        linked, but no USB host (power-only port or charger)
//       white         typing
// BOOT: short press   USB serial on/off (cyan flashes, restarts)
//       hold 3 s      forget the pad (red flashes, restarts)
#include <Arduino.h>
#include <USB.h>
#include <USBHIDKeyboard.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h>
#include <stdarg.h>
#include "macrolink.h"

#define BTN_PIN        0
#define SCAN_DWELL_MS  80    // per channel while searching: a full sweep takes about 1 s
#define MAX_CHANNEL    13
#define LONG_PRESS_MS  3000

static USBHIDKeyboard Keyboard;
static USBCDC *s_serial = nullptr;
static Preferences prefs;

static const uint8_t BROADCAST[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static uint8_t s_pad[6];
static volatile bool s_paired = false;
static volatile bool s_newPair = false;  // set by the callback, saved by loop()
static volatile bool s_linked = false;
static volatile uint32_t s_lastPong = 0;
static int s_channel = 0;                // scan position
static volatile uint8_t s_padChannel = 0;  // channel the pad answered on

static QueueHandle_t s_jobs;
static volatile bool s_busy = false;
static volatile bool s_haveSeq = false;
static volatile uint16_t s_lastSeq = 0;   // last job accepted (repeats are not typed again)
static volatile int s_curStep = -1;

static void dlog(const char *fmt, ...) {
  if (!s_serial || !*s_serial) return;
  char buf[128];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  s_serial->println(buf);
}

static void add_peer(const uint8_t *mac) {
  if (esp_now_is_peer_exist(mac)) return;
  esp_now_peer_info_t p = {};
  memcpy(p.peer_addr, mac, 6);
  p.channel = 0;  // current channel
  p.ifidx = WIFI_IF_STA;
  esp_now_add_peer(&p);
}

static void send_ping(const uint8_t *to) {
  LinkPing p;
  link_header(p.h, LINK_PING);
  p.usb = (bool)USB;
  p.busy = s_busy;
  p.unpaired = !s_paired;
  esp_now_send(to, (const uint8_t *)&p, sizeof(p));
}

static void send_status(uint16_t seq, int step, bool done) {
  LinkStatus st;
  link_header(st.h, LINK_STATUS);
  st.seq = seq;
  st.step = (int8_t)step;
  st.done = done;
  st.usb = (bool)USB;
  esp_now_send(s_pad, (const uint8_t *)&st, sizeof(st));
}

static void on_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  uint8_t type = link_type(data, len);
  if (!type) return;
  const uint8_t *mac = info->src_addr;
  bool fromPad = s_paired && !memcmp(mac, s_pad, 6);

  if (type == LINK_PONG) {
    const LinkPong *p = (const LinkPong *)data;
    if (!fromPad && !(!s_paired && p->pairing)) return;
    if (!s_paired) {
      memcpy(s_pad, mac, 6);
      s_paired = true;
      s_newPair = true;
    }
    s_padChannel = info->rx_ctrl->channel;  // the scan may have hopped on since the PING
    s_lastPong = millis();
    s_linked = true;
  } else if (type == LINK_JOB && fromPad) {
    const LinkJobMsg *m = (const LinkJobMsg *)data;
    s_lastPong = millis();
    if (s_haveSeq && m->seq == s_lastSeq) {  // a resend: the pad missed our STATUS
      send_status(m->seq, s_busy ? s_curStep : -1, !s_busy);
      return;
    }
    if (s_busy) return;  // the pad only sends one job at a time
    s_lastSeq = m->seq;
    s_haveSeq = true;
    s_busy = true;
    if (xQueueSend(s_jobs, m, 0) != pdTRUE) s_busy = false;
  }
}

static uint16_t s_typingSeq;
static void on_step(int i) {
  s_curStep = i;
  send_status(s_typingSeq, i, false);
}

static void type_task(void *) {
  LinkJobMsg m;
  for (;;) {
    if (xQueueReceive(s_jobs, &m, portMAX_DELAY) != pdTRUE) continue;
    s_typingSeq = m.seq;
    s_curStep = -1;
    send_status(m.seq, -1, false);  // acknowledge before the first key
    macro_run(Keyboard, m.job, on_step);
    s_busy = false;
    s_curStep = -1;
    send_status(m.seq, -1, true);
    dlog("typed %s '%s'", m.job.kind == MJ_TEXT ? "text" : "code", m.job.data);
  }
}

// ---------------------------------------------------------------- LED

static void led(uint8_t r, uint8_t g, uint8_t b) {
  static uint32_t last = 0xFFFFFFFF;
  uint32_t c = (r << 16) | (g << 8) | b;
  if (c == last) return;
  last = c;
  rgbLedWrite(LED_PIN, r, g, b);
}

static void led_flash(uint8_t r, uint8_t g, uint8_t b, int times) {
  for (int i = 0; i < times; i++) {
    led(r, g, b);
    delay(150);
    led(0, 0, 0);
    delay(150);
  }
}

static void led_update() {
  bool blinkOn = (millis() / 250) % 2;
  if (s_busy) led(40, 40, 40);
  else if (!s_paired) led(blinkOn ? 30 : 0, 0, blinkOn ? 30 : 0);
  else if (!s_linked) led(0, 0, (millis() / 500) % 2 ? 40 : 0);
  else if ((bool)USB) led(0, 25, 0);
  else led(30, 20, 0);
}

// ---------------------------------------------------------------- button

static void handle_button() {
  static uint32_t downAt = 0;
  bool down = digitalRead(BTN_PIN) == LOW;
  if (down && !downAt) downAt = millis() | 1;
  if (down && downAt && millis() - downAt > LONG_PRESS_MS) {
    prefs.remove("pad");
    dlog("forgot the pad, restarting");
    led_flash(40, 0, 0, 4);
    ESP.restart();
  }
  if (!down && downAt) {
    if (millis() - downAt > 50) {
      bool serial = !prefs.getUChar("serial", 1);
      prefs.putUChar("serial", serial);
      led_flash(0, 30, 30, serial ? 3 : 1);
      ESP.restart();
    }
    downAt = 0;
  }
}

// ---------------------------------------------------------------- serial

static void handle_serial() {
  static char line[32];
  static int n = 0;
  if (!s_serial) return;
  while (s_serial->available()) {
    char c = s_serial->read();
    if (c != '\n' && c != '\r') {
      if (n < (int)sizeof(line) - 1) line[n++] = c;
      continue;
    }
    line[n] = 0;
    n = 0;
    if (!strcmp(line, "status"))
      s_serial->printf("OK usb=%d paired=%d linked=%d ch=%d busy=%d pad=%02x:%02x:%02x:%02x:%02x:%02x\n", (bool)USB,
                       s_paired, s_linked, WiFi.channel(), s_busy, s_pad[0], s_pad[1], s_pad[2], s_pad[3], s_pad[4],
                       s_pad[5]);
  }
}

// ----------------------------------------------------------------

void setup() {
  pinMode(BTN_PIN, INPUT_PULLUP);
  prefs.begin("dongle", false);
  if (prefs.getUChar("serial", 1)) s_serial = new USBCDC(0);  // must exist before USB.begin()
  Keyboard.begin();
  USB.begin();
  if (s_serial) s_serial->begin(115200);

  if (prefs.getBytes("pad", s_pad, 6) == 6) s_paired = true;

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.setSleep(false);  // never doze: we must hear the pad at any moment
  esp_wifi_set_country_code(WIFI_COUNTRY, false);
  esp_now_init();
  esp_now_register_recv_cb(on_recv);
  add_peer(BROADCAST);
  if (s_paired) add_peer(s_pad);

  s_jobs = xQueueCreate(1, sizeof(LinkJobMsg));
  xTaskCreatePinnedToCore(type_task, "type", 4096, nullptr, 5, nullptr, 1);
  dlog("MacroDiver dongle ready, %s", s_paired ? "paired" : "not paired");
}

void loop() {
  static uint32_t lastTx = 0;
  uint32_t now = millis();

  if (s_newPair) {
    s_newPair = false;
    prefs.putBytes("pad", s_pad, 6);
    add_peer(s_pad);
    dlog("paired with pad %02x:%02x:%02x:%02x:%02x:%02x", s_pad[0], s_pad[1], s_pad[2], s_pad[3], s_pad[4], s_pad[5]);
  }

  if (s_linked && now - s_lastPong > LINK_TIMEOUT_MS && !s_busy) {
    s_linked = false;
    dlog("lost the pad, searching");
  }
  if (!s_linked) {
    // Hop channels until the pad answers: it sits on its home network's channel
    if (now - lastTx >= SCAN_DWELL_MS) {
      lastTx = now;
      s_channel = s_channel % MAX_CHANNEL + 1;
      if (esp_wifi_set_channel(s_channel, WIFI_SECOND_CHAN_NONE) == ESP_OK) send_ping(BROADCAST);
    }
  } else if (s_channel != s_padChannel) {
    s_channel = s_padChannel;
    esp_wifi_set_channel(s_channel, WIFI_SECOND_CHAN_NONE);
    dlog("linked on channel %d", s_channel);
  } else if (now - lastTx >= LINK_PING_MS) {
    lastTx = now;
    send_ping(s_pad);  // heartbeat, and keeps the pad's USB status fresh
  }

  handle_button();
  handle_serial();
  led_update();
  delay(5);
}
