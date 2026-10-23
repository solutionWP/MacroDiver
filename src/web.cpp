// Wi-Fi + web editor. Joins the saved home network (station) and serves the
// editor there; falls back to its own WPA2 setup network with a captive portal.
//   GET  /              editor page
//   GET  /api/state     catalog + builds + messages (JSON)
//   GET  /icons         small icons, raw, in the order given by "i" in /api/state
//   POST /api/save      {active, builds:[{name,layout,ids}], msgs:[{id,text}]}
//   POST /api/custom    {id (0 = new), name, cat, code, src} -> {ok, id}
//   POST /api/custom/delete {id}
//   POST /api/icon?id=&sha=  multipart: icon pixels (icons.h), for a custom or built-in stratagem
//   GET  /api/wifi      {state, home, online, ip, apOn, apSsid, host}
//   GET  /api/scan      {busy} or {nets:[{ssid,rssi,lock}]} (async scan)
//   POST /api/wifi      {ssid, pass} -> save and connect
//   POST /api/wifi/forget
#include "web.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <esp_random.h>
#include <esp_mac.h>
#include "storage.h"
#include "stratagems.h"
#include "icons.h"
#include "hid_out.h"
#include "link.h"
#include "web_page.h"

#define HOSTNAME        "macrodiver"
#define STA_TIMEOUT_MS  20000   // then open the setup network
#define STA_RETRY_MS    120000  // retry the home network while nobody uses the setup network
#define AP_LINGER_MS    60000   // keep the setup network after going online, while unused

static WebServer *s_server = nullptr;
static DNSServer *s_dns = nullptr;
static bool s_enabled = false, s_apOn = false, s_staPaused = false;
static uint32_t s_staStart = 0, s_apIdleSince = 0, s_lastTick = 0;
static char s_apSsid[24], s_apPass[16], s_home[33], s_homePass[65];
static char s_ip[16] = "", s_url[40] = "";
static uint32_t s_saves = 0;

// Names and messages are shown with ASCII-only fonts and typed via an ASCII
// keymap: keep printable ASCII, drop the rest.
static void ascii_copy(char *dst, const char *src, size_t maxLen) {
  size_t n = 0;
  for (; *src && n < maxLen; src++)
    if (*src >= 32 && *src <= 126) dst[n++] = *src;
  dst[n] = 0;
  while (n && dst[n - 1] == ' ') dst[--n] = 0;
}

// The state document (~170 stratagems with icon paths) is big for the internal heap
struct PsramAllocator : ArduinoJson::Allocator {
  void *allocate(size_t n) override { return ps_malloc(n); }
  void deallocate(void *p) override { free(p); }
  void *reallocate(void *p, size_t n) override { return ps_realloc(p, n); }
};
static PsramAllocator s_psram;

String web_state_json() {
  JsonDocument doc(&s_psram);
  doc["maxSlots"] = MAX_SLOTS;
  doc["maxBuilds"] = MAX_BUILDS;
  doc["maxMsgs"] = MAX_MSG;
  doc["nameLen"] = NAME_LEN;
  doc["msgLen"] = MSG_LEN;
  doc["iconSize"] = icon_small_size();
  doc["iconVer"] = icons_version();
  doc["maxCustom"] = MAX_CUSTOM;
  doc["codeLen"] = MAX_CODE;
  doc["active"] = g_settings.activeBuild;
  JsonArray cats = doc["cats"].to<JsonArray>();
  for (int c = 0; c < CAT_COUNT; c++) {
    JsonObject o = cats.add<JsonObject>();
    o["n"] = cat_name(c);
    char col[8];
    snprintf(col, sizeof(col), "#%06lX", (unsigned long)cat_color(c));
    o["c"] = col;
  }
  JsonArray strats = doc["strats"].to<JsonArray>();
  auto addStrat = [&](const Strat *s, const char *path) {
    JsonObject o = strats.add<JsonObject>();
    o["id"] = s->id;
    o["c"] = s->cat;
    o["n"] = s->name;
    o["k"] = s->code;
    o["i"] = icon_index(s->id);
    o["p"] = path;              // icon library path, "" if unknown
    o["h"] = icon_sha(s->id);   // SHA of the icon the pad has, "" if none
  };
  for (size_t i = 0; i < strat_catalog_count(); i++) {
    const Strat *s = strat_catalog_at(i);
    addStrat(s, icon_path(s->id));
  }
  for (int j = 0; j < g_customCount; j++) addStrat(custom_find(g_customs[j].id), g_customs[j].src);
  JsonArray builds = doc["builds"].to<JsonArray>();
  for (int i = 0; i < g_buildCount; i++) {
    JsonObject o = builds.add<JsonObject>();
    o["name"] = g_builds[i].name;
    o["layout"] = g_builds[i].layout;
    JsonArray ids = o["ids"].to<JsonArray>();
    for (int k = 0; k < g_builds[i].count; k++) ids.add(g_builds[i].ids[k]);
  }
  JsonArray msgs = doc["msgs"].to<JsonArray>();
  for (int j = 0; j < g_msgCount; j++) {
    JsonObject o = msgs.add<JsonObject>();
    o["id"] = g_msgs[j].id;
    o["text"] = g_msgs[j].text;
  }
  String out;
  serializeJson(doc, out);
  return out;
}

static void send_state() {
  s_server->sendHeader("Cache-Control", "no-store");
  s_server->send(200, "application/json", web_state_json());
}

static void send_icons() {
  const int px = icon_small_size();
  const size_t each = (size_t)px * px * 3;
  s_server->sendHeader("Cache-Control", "max-age=86400");
  s_server->setContentLength(each * icon_count());
  s_server->send(200, "application/octet-stream", "");
  for (int i = 0; i < icon_count(); i++) s_server->sendContent((const char *)icon_small_data(i), each);
}

const char *web_apply_json(const char *json) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return "bad json";
  JsonArray builds = doc["builds"];
  if (builds.isNull() || builds.size() == 0) return "need at least one build";

  // Messages first: builds may reference new message IDs
  int nm = 0;
  ChatMsg msgs[MAX_MSG];
  for (JsonObject m : doc["msgs"].as<JsonArray>()) {
    if (nm >= MAX_MSG) break;
    int id = m["id"] | 0;
    if (id < MSG_ID_BASE || id >= MSG_ID_BASE + 1000) continue;
    bool dup = false;
    for (int k = 0; k < nm; k++) dup |= msgs[k].id == id;
    if (dup) continue;
    msgs[nm].id = id;
    ascii_copy(msgs[nm].text, m["text"] | "", MSG_LEN);
    if (msgs[nm].text[0]) nm++;
  }
  memcpy(g_msgs, msgs, sizeof(ChatMsg) * nm);
  g_msgCount = nm;
  for (int j = 0; j < g_msgCount; j++) msg_refresh(j);

  g_buildCount = 0;
  for (JsonObject b : builds) {
    if (g_buildCount >= MAX_BUILDS) break;
    char name[NAME_LEN + 1];
    ascii_copy(name, b["name"] | "", NAME_LEN);
    if (!name[0]) snprintf(name, sizeof(name), "Build %d", g_buildCount + 1);
    int i = build_add(name);
    Build &nb = g_builds[i];
    nb.layout = (b["layout"] | 0) ? LAYOUT_DENSE : LAYOUT_LARGE;
    for (JsonVariant v : b["ids"].as<JsonArray>()) {
      uint16_t id = v.as<uint16_t>();
      if (nb.count < MAX_SLOTS && strat_get(id) && build_find_slot(&nb, id) < 0) nb.ids[nb.count++] = id;
    }
  }
  int active = doc["active"] | 0;
  g_settings.activeBuild = (active >= 0 && active < g_buildCount) ? active : 0;

  storage_save_all_builds();
  storage_save_msgs();
  storage_save_settings();
  s_saves++;
  hid_log("Web save: %d builds, %d messages", g_buildCount, g_msgCount);
  return nullptr;
}

static void reply(int code, const char *err) {
  if (!err) {
    s_server->send(code, "application/json", "{\"ok\":true}");
    return;
  }
  JsonDocument doc;
  doc["ok"] = false;
  doc["err"] = err;
  String out;
  serializeJson(doc, out);
  s_server->send(code, "application/json", out);
}

// POST /api/custom {id (0 = new), name, cat, code, src} -> {ok, id}
static void handle_custom() {
  JsonDocument doc;
  if (deserializeJson(doc, s_server->arg("plain"))) return reply(400, "bad json");
  char name[NAME_LEN + 1], code[MAX_CODE + 1];
  ascii_copy(name, doc["name"] | "", NAME_LEN);
  if (!name[0]) return reply(400, "need a name");
  size_t n = 0;
  for (const char *c = doc["code"] | ""; *c && n < MAX_CODE; c++)
    if (strchr("UDLR", *c)) code[n++] = *c;
  code[n] = 0;
  int cat = doc["cat"] | (int)CAT_CUSTOM;
  if (cat < 0 || cat >= CAT_MESSAGE) cat = CAT_CUSTOM;
  const char *src = doc["src"] | "";
  if (strlen(src) > SRC_LEN) src = "";  // too long to keep: only update tracking is lost

  int id = doc["id"] | 0, j;
  if (id) {
    j = custom_index(id);
    if (j < 0) return reply(404, "unknown stratagem");
    CustomStrat &c = g_customs[j];
    strcpy(c.name, name);
    strcpy(c.code, code);
    c.cat = cat;
    if (doc["src"].is<const char *>()) strcpy(c.src, src);
    custom_refresh(j);
  } else {
    j = custom_add(name, code, cat, src);
    if (j < 0) return reply(409, "custom list is full");
  }
  storage_save_customs();
  s_saves++;
  hid_log("Web: custom %u '%s' %s", g_customs[j].id, g_customs[j].name, g_customs[j].code);
  char out[40];
  snprintf(out, sizeof(out), "{\"ok\":true,\"id\":%u}", g_customs[j].id);
  s_server->send(200, "application/json", out);
}

// POST /api/custom/delete {id}
static void handle_custom_delete() {
  JsonDocument doc;
  if (deserializeJson(doc, s_server->arg("plain"))) return reply(400, "bad json");
  int j = custom_index(doc["id"] | 0);
  if (j < 0) return reply(404, "unknown stratagem");
  custom_delete(j);  // also from builds, and its icon
  storage_save_customs();
  s_saves++;
  reply(200, nullptr);
}

// POST /api/icon?id=<id>&sha=<library blob sha>, multipart file of ICON_BYTES
// (64 px then 44 px, RGB565 big-endian + alpha), rasterised by the page.
static uint8_t *s_up = nullptr;
static size_t s_upLen = 0;
static bool s_upBad = false;

static void icon_upload() {
  HTTPUpload &u = s_server->upload();
  if (u.status == UPLOAD_FILE_START) {
    if (!s_up) s_up = (uint8_t *)ps_malloc(ICON_BYTES);
    s_upLen = 0;
    s_upBad = !s_up;
  } else if (u.status == UPLOAD_FILE_WRITE && !s_upBad) {
    if (s_upLen + u.currentSize > ICON_BYTES) s_upBad = true;
    else {
      memcpy(s_up + s_upLen, u.buf, u.currentSize);
      s_upLen += u.currentSize;
    }
  }
}

static void icon_done() {
  uint16_t id = s_server->arg("id").toInt();
  const Strat *s = strat_get(id);
  if (!s || s->cat == CAT_MESSAGE) return reply(404, "unknown stratagem");
  if (s_upBad || s_upLen != ICON_BYTES) return reply(400, "bad icon data");
  if (!icon_store(id, s_server->arg("sha").c_str(), s_up, s_upLen)) return reply(507, "icon storage full");
  s_saves++;
  reply(200, nullptr);
}

static void handle_save() {
  const char *err = web_apply_json(s_server->arg("plain").c_str());
  if (err) s_server->send(400, "application/json", String("{\"ok\":false,\"err\":\"") + err + "\"}");
  else s_server->send(200, "application/json", "{\"ok\":true}");
}

static void redirect_home() {  // captive portal: any unknown URL -> editor
  String host = (s_apOn && WiFi.status() != WL_CONNECTED) ? WiFi.softAPIP().toString() : String(s_ip);
  s_server->sendHeader("Location", "http://" + host + "/");
  s_server->send(302, "text/plain", "");
}

static const char *state_name() {
  switch (web_state()) {
    case NET_CONNECTING: return "connecting";
    case NET_ONLINE: return "online";
    case NET_SETUP: return "setup";
    default: return "off";
  }
}

static void send_wifi() {
  JsonDocument doc;
  bool online = WiFi.status() == WL_CONNECTED;
  doc["state"] = state_name();
  doc["home"] = s_home;
  doc["online"] = online;
  doc["ip"] = online ? WiFi.localIP().toString() : String("");
  doc["apOn"] = s_apOn;
  doc["apSsid"] = s_apSsid;
  doc["host"] = HOSTNAME ".local";
  String out;
  serializeJson(doc, out);
  s_server->sendHeader("Cache-Control", "no-store");
  s_server->send(200, "application/json", out);
}

static void send_scan() {
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_FAILED) {
    WiFi.scanNetworks(true);  // async
    n = WIFI_SCAN_RUNNING;
  }
  if (n == WIFI_SCAN_RUNNING) {
    s_server->send(200, "application/json", "{\"busy\":true}");
    return;
  }
  JsonDocument doc;
  JsonArray nets = doc["nets"].to<JsonArray>();
  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    if (!ssid.length()) continue;
    bool dup = false;  // same network on several access points / bands
    for (JsonObject o : nets) dup |= ssid == o["ssid"].as<const char *>();
    if (dup) continue;
    JsonObject o = nets.add<JsonObject>();
    o["ssid"] = ssid;
    o["rssi"] = WiFi.RSSI(i);
    o["lock"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
  }
  WiFi.scanDelete();
  String out;
  serializeJson(doc, out);
  s_server->send(200, "application/json", out);
}

static void sta_connect() {
  if (!s_home[0]) return;
  WiFi.setAutoReconnect(true);
  WiFi.begin(s_home, s_homePass);
  s_staStart = millis();
  s_staPaused = false;
  hid_log("Wi-Fi: joining %s", s_home);
}

static void handle_wifi_set() {
  JsonDocument doc;
  if (deserializeJson(doc, s_server->arg("plain")) || !(doc["ssid"] | "")[0]) {
    s_server->send(400, "application/json", "{\"ok\":false,\"err\":\"need a network name\"}");
    return;
  }
  strncpy(s_home, doc["ssid"] | "", sizeof(s_home) - 1);
  s_home[sizeof(s_home) - 1] = 0;
  strncpy(s_homePass, doc["pass"] | "", sizeof(s_homePass) - 1);
  s_homePass[sizeof(s_homePass) - 1] = 0;
  storage_put_str("wssid", s_home);
  storage_put_str("wpass", s_homePass);
  s_server->send(200, "application/json", "{\"ok\":true}");
  WiFi.disconnect(false);
  sta_connect();
}

static void ap_start() {
  if (s_apOn) return;
  WiFi.mode(WIFI_AP_STA);  // STA side stays available for scans / joining
  WiFi.softAP(s_apSsid, s_apPass, 6, 0, 2);
  s_dns = new DNSServer();
  s_dns->start(53, "*", WiFi.softAPIP());
  s_apOn = true;
  s_apIdleSince = millis();
  hid_log("Wi-Fi: setup network %s / %s", s_apSsid, s_apPass);
}

static void ap_stop() {
  if (!s_apOn) return;
  s_dns->stop();
  delete s_dns;
  s_dns = nullptr;
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  s_apOn = false;
  hid_log("Wi-Fi: setup network off");
}

void web_forget_home() {
  s_home[0] = s_homePass[0] = 0;
  storage_put_str("wssid", "");
  storage_put_str("wpass", "");
  if (s_enabled) {
    WiFi.disconnect(false);
    ap_start();
  }
}

static void handle_wifi_forget() {
  s_server->send(200, "application/json", "{\"ok\":true}");
  web_forget_home();
}

static void start() {
  if (s_enabled) return;
  uint8_t mac[6];
  esp_efuse_mac_get_default(mac);
  snprintf(s_apSsid, sizeof(s_apSsid), "MacroDiver-%02X%02X", mac[4], mac[5]);
  // Setup-network password: generated once and kept, so phones remember the network
  storage_get_str("appass", s_apPass, sizeof(s_apPass));
  if (strlen(s_apPass) < 8) {
    static const char ALPHA[] = "abcdefghjkmnpqrstuvwxyz23456789";  // no look-alikes
    for (int i = 0; i < 8; i++) s_apPass[i] = ALPHA[esp_random() % (sizeof(ALPHA) - 1)];
    s_apPass[8] = 0;
    storage_put_str("appass", s_apPass);
  }
  storage_get_str("wssid", s_home, sizeof(s_home));
  storage_get_str("wpass", s_homePass, sizeof(s_homePass));

  WiFi.persistent(false);  // credentials live in our own storage
  WiFi.setHostname(HOSTNAME);
  WiFi.mode(WIFI_STA);
  if (s_home[0]) sta_connect();
  else ap_start();

  s_server = new WebServer(80);
  s_server->on("/", HTTP_GET, [] {
    s_server->sendHeader("Cache-Control", "no-store");
    s_server->send_P(200, "text/html", WEB_PAGE);
  });
  s_server->on("/api/state", HTTP_GET, send_state);
  s_server->on("/icons", HTTP_GET, send_icons);
  s_server->on("/api/save", HTTP_POST, handle_save);
  s_server->on("/api/custom", HTTP_POST, handle_custom);
  s_server->on("/api/custom/delete", HTTP_POST, handle_custom_delete);
  s_server->on("/api/icon", HTTP_POST, icon_done, icon_upload);
  s_server->on("/api/wifi", HTTP_GET, send_wifi);
  s_server->on("/api/scan", HTTP_GET, send_scan);
  s_server->on("/api/wifi", HTTP_POST, handle_wifi_set);
  s_server->on("/api/wifi/forget", HTTP_POST, handle_wifi_forget);
  s_server->onNotFound(redirect_home);
  s_server->begin();
  if (MDNS.begin(HOSTNAME)) MDNS.addService("http", "tcp", 80);
  s_enabled = true;
}

static void stop() {
  if (!s_enabled) return;
  MDNS.end();
  s_server->stop();
  delete s_server;
  s_server = nullptr;
  ap_stop();
  s_ip[0] = s_url[0] = 0;
  s_enabled = false;
  if (link_enabled()) {  // the dongle link still needs the radio
    WiFi.disconnect(false);
    link_wifi_changed();
  } else {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  }
  hid_log("Wi-Fi: off");
}

void web_begin() {
  if (g_settings.wifiOn) start();
}

void web_set_enabled(bool on) {
  g_settings.wifiOn = on;
  storage_save_settings();
  if (on) start();
  else stop();
}

void web_loop() {
  if (!s_enabled) return;
  if (s_apOn) s_dns->processNextRequest();
  s_server->handleClient();

  uint32_t now = millis();
  if (now - s_lastTick < 500) return;
  s_lastTick = now;
  bool online = WiFi.status() == WL_CONNECTED;

  if (!online && s_home[0] && !s_staPaused && now - s_staStart > STA_TIMEOUT_MS) {
    // Home network unreachable: stop retrying (its scans disturb the setup
    // network's channel) and open the setup network instead.
    WiFi.setAutoReconnect(false);
    WiFi.disconnect(false);
    s_staPaused = true;
    ap_start();
  }
  if (!online && !s_home[0]) ap_start();
  if (s_staPaused && s_home[0] && WiFi.softAPgetStationNum() == 0 && now - s_staStart > STA_RETRY_MS) sta_connect();

  if (online && s_apOn) {
    if (WiFi.softAPgetStationNum() > 0) s_apIdleSince = now;
    else if (now - s_apIdleSince > AP_LINGER_MS) ap_stop();
  }

  String addr = online ? WiFi.localIP().toString() : (s_apOn ? WiFi.softAPIP().toString() : String(""));
  if (strcmp(addr.c_str(), s_ip)) {
    strncpy(s_ip, addr.c_str(), sizeof(s_ip) - 1);
    s_ip[sizeof(s_ip) - 1] = 0;
    if (s_ip[0]) snprintf(s_url, sizeof(s_url), "http://%s/", s_ip);
    else s_url[0] = 0;
    if (online) hid_log("Wi-Fi: online at %s (%s)", s_ip, s_home);
  }
}

bool web_enabled() { return s_enabled; }
NetState web_state() {
  if (!s_enabled) return NET_OFF;
  if (WiFi.status() == WL_CONNECTED) return NET_ONLINE;
  if (s_home[0] && !s_staPaused) return NET_CONNECTING;
  return NET_SETUP;
}
const char *web_ip() { return s_ip; }
const char *web_url() { return s_url; }
const char *web_home_ssid() { return s_home; }
bool web_ap_on() { return s_apOn; }
const char *web_ap_ssid() { return s_apSsid; }
const char *web_ap_pass() { return s_apPass; }
int web_ap_clients() { return s_apOn ? WiFi.softAPgetStationNum() : 0; }
uint32_t web_save_count() { return s_saves; }
