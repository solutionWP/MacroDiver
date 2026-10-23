// Pad side of the dongle link (protocol in lib/macrolink/macrolink.h).
// PING and STATUS are handled in the ESP-NOW receive callback (Wi-Fi task) so
// the dongle's channel scan gets its PONG without waiting for the UI loop;
// link_loop() does the retries and saves a new pairing.
#include "link.h"
#include "storage.h"
#include "hid_out.h"
#include "web.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_random.h>

#define JOB_RETRY_MS 250   // resend a job the dongle hasn't acknowledged
#define JOB_TRIES    4
#define JOB_STALL_MS 3000  // no STATUS for this long while typing = give up

static bool s_on = false;
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;  // s_dongle / s_paired
static uint8_t s_dongle[6];
static volatile bool s_paired = false;
static volatile bool s_newPair = false;  // set by the callback, saved by link_loop()
static volatile uint32_t s_lastSeen = 0;
static volatile bool s_usb = false;
static volatile uint32_t s_pairUntil = 0;
static volatile uint8_t s_channel = 0;  // current Wi-Fi channel, refreshed by link_loop()

static LinkJobMsg s_job;
static uint16_t s_seq;
static volatile bool s_pending = false, s_acked = false;
static volatile int s_step = -1;
static volatile uint32_t s_lastStatus = 0;
static uint32_t s_sentAt = 0;
static int s_tries = 0;

static bool recent(uint32_t t, uint32_t ms) { return t && millis() - t < ms; }
static bool pairing() { return s_pairUntil && (int32_t)(millis() - s_pairUntil) < 0; }

static void add_peer(const uint8_t *mac) {
  if (esp_now_is_peer_exist(mac)) return;
  esp_now_peer_info_t p = {};
  memcpy(p.peer_addr, mac, 6);
  p.channel = 0;  // whatever channel the pad is on
  p.ifidx = WIFI_IF_STA;
  esp_now_add_peer(&p);
}

static void on_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  uint8_t type = link_type(data, len);
  if (!type) return;
  const uint8_t *mac = info->src_addr;
  portENTER_CRITICAL(&s_mux);
  bool known = s_paired && !memcmp(mac, s_dongle, 6);
  portEXIT_CRITICAL(&s_mux);

  if (type == LINK_PING) {
    const LinkPing *p = (const LinkPing *)data;
    bool adopt = p->unpaired && pairing();
    if (!adopt && (!known || p->unpaired)) return;  // not ours, or ours but asking to pair
    if (adopt) {
      portENTER_CRITICAL(&s_mux);
      bool replace = s_paired && memcmp(mac, s_dongle, 6);
      uint8_t old[6];
      memcpy(old, s_dongle, 6);
      memcpy(s_dongle, mac, 6);
      s_paired = true;
      portEXIT_CRITICAL(&s_mux);
      if (replace) esp_now_del_peer(old);
      add_peer(mac);
      s_pairUntil = 0;
      s_newPair = true;
    }
    s_lastSeen = millis();
    s_usb = p->usb;
    LinkPong pong;
    link_header(pong.h, LINK_PONG);
    pong.pairing = adopt;
    pong.channel = s_channel;
    esp_now_send(mac, (const uint8_t *)&pong, sizeof(pong));
  } else if (type == LINK_STATUS && known) {
    const LinkStatus *st = (const LinkStatus *)data;
    s_lastSeen = millis();
    s_usb = st->usb;
    if (s_pending && st->seq == s_job.seq) {
      s_acked = true;
      s_lastStatus = millis();
      s_step = st->done ? -1 : st->step;
      if (st->done) s_pending = false;
    }
  }
}

static void idle_channel() {
  // Without the editor nothing else picks a channel: use a fixed one
  if (!web_enabled()) esp_wifi_set_channel(LINK_IDLE_CHANNEL, WIFI_SECOND_CHAN_NONE);
}

static void load_pairing() {
  char buf[18];
  storage_get_str("dongle", buf, sizeof(buf));
  uint8_t m[6];
  if (sscanf(buf, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) != 6) return;
  memcpy(s_dongle, m, 6);
  s_paired = true;
  add_peer(m);
}

static void start() {
  if (s_on) return;
  WiFi.persistent(false);
  WiFi.setSleep(false);  // modem sleep would miss the dongle's packets
  if (WiFi.getMode() == WIFI_OFF) {
    WiFi.mode(WIFI_STA);
    idle_channel();
  }
  if (esp_now_init() != ESP_OK) {
    hid_log("Link: ESP-NOW init failed");
    return;
  }
  esp_now_register_recv_cb(on_recv);
  load_pairing();
  s_seq = (uint16_t)esp_random();  // a restarted pad never repeats the dongle's last seq
  s_on = true;
  hid_log("Link: on, channel %d, %s", WiFi.channel(), s_paired ? "paired" : "not paired");
}

static void stop() {
  if (!s_on) return;
  esp_now_deinit();
  s_on = false;
  s_pending = false;
  s_paired = false;
  s_pairUntil = 0;
  s_lastSeen = 0;
  WiFi.setSleep(true);
  if (!web_enabled()) WiFi.mode(WIFI_OFF);
  hid_log("Link: off");
}

void link_begin() {
  if (g_settings.output == 1) start();
}

void link_set_enabled(bool on) {
  g_settings.output = on;
  storage_save_settings();
  if (on) start();
  else stop();
}

bool link_enabled() { return s_on; }

void link_wifi_changed() {
  if (s_on) idle_channel();
}

void link_loop() {
  if (!s_on) return;
  uint8_t ch;
  wifi_second_chan_t second;
  if (esp_wifi_get_channel(&ch, &second) == ESP_OK) s_channel = ch;  // read here: not from the Wi-Fi task
  if (s_newPair) {
    s_newPair = false;
    char buf[18];
    snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x", s_dongle[0], s_dongle[1], s_dongle[2], s_dongle[3],
             s_dongle[4], s_dongle[5]);
    storage_put_str("dongle", buf);
    hid_log("Link: paired with dongle %s", buf);
  }
  if (s_pairUntil && !pairing()) s_pairUntil = 0;

  if (!s_pending) return;
  uint32_t now = millis();
  if (!s_acked && now - s_sentAt > JOB_RETRY_MS) {
    if (++s_tries > JOB_TRIES) {
      s_pending = false;
      hid_log("Link: dongle did not answer");
      return;
    }
    s_sentAt = now;
    esp_now_send(s_dongle, (const uint8_t *)&s_job, sizeof(s_job));
  } else if (s_acked && now - s_lastStatus > JOB_STALL_MS) {
    s_pending = false;
    s_step = -1;
    hid_log("Link: dongle stopped reporting");
  }
}

bool link_dongle_seen() { return s_on && s_paired && recent(s_lastSeen, LINK_TIMEOUT_MS); }
bool link_connected() { return link_dongle_seen() && s_usb; }
bool link_paired() { return s_paired; }

bool link_send(const MacroJob &job) {
  if (!link_connected() || s_pending) return false;
  link_header(s_job.h, LINK_JOB);
  s_job.seq = ++s_seq;
  s_job.job = job;
  s_acked = false;
  s_step = -1;
  s_tries = 1;
  s_sentAt = millis();
  s_pending = true;
  if (esp_now_send(s_dongle, (const uint8_t *)&s_job, sizeof(s_job)) != ESP_OK) {
    s_pending = false;
    return false;
  }
  return true;
}

bool link_busy() { return s_pending; }
int link_step() { return s_step; }

void link_pair_start() {
  if (s_on) s_pairUntil = (millis() + LINK_PAIR_MS) | 1;
}
void link_pair_cancel() { s_pairUntil = 0; }
int link_pair_seconds_left() { return pairing() ? (int)((s_pairUntil - millis()) / 1000) + 1 : 0; }
