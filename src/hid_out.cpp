#include "hid_out.h"
#include "storage.h"
#include "link.h"
#include <Arduino.h>
#include <USB.h>
#include <USBHIDKeyboard.h>
#include <stdarg.h>

// HID usage IDs (USB HID Usage Tables, keyboard page). 0xE0..0xE7 are modifiers.
enum : uint8_t {
  K_A = 0x04, K_D = 0x07, K_E = 0x08, K_F = 0x09, K_I = 0x0C, K_J = 0x0D, K_K = 0x0E, K_L = 0x0F,
  K_Q = 0x14, K_S = 0x16, K_W = 0x1A, K_TAB = 0x2B, K_SPACE = 0x2C, K_CAPS = 0x39,
  K_RIGHT = 0x4F, K_LEFT = 0x50, K_DOWN = 0x51, K_UP = 0x52,
  K_T = 0x17, K_Y = 0x1C, K_ENTER = 0x28,
  K_KP2 = 0x5A, K_KP4 = 0x5C, K_KP6 = 0x5E, K_KP8 = 0x60,
  K_LCTRL = 0xE0, K_LSHIFT = 0xE1, K_LALT = 0xE2,
};

const KeyOption MENU_KEYS[] = {
  {"L-Ctrl", K_LCTRL}, {"L-Shift", K_LSHIFT}, {"L-Alt", K_LALT}, {"Space", K_SPACE},
  {"Tab", K_TAB}, {"Caps Lock", K_CAPS}, {"Q", K_Q}, {"E", K_E}, {"F", K_F},
};
const size_t MENU_KEY_COUNT = sizeof(MENU_KEYS) / sizeof(MENU_KEYS[0]);

const DirSet DIR_SETS[] = {
  {"W A S D", K_W, K_S, K_A, K_D},
  {"Arrows", K_UP, K_DOWN, K_LEFT, K_RIGHT},
  {"I J K L", K_I, K_K, K_J, K_L},
  {"Numpad 8456", K_KP8, K_KP2, K_KP4, K_KP6},
};
const size_t DIR_SET_COUNT = sizeof(DIR_SETS) / sizeof(DIR_SETS[0]);

// openMs: wait after opening the stratagem menu; press/gap: per arrow
const SpeedOption SPEEDS[] = {
  {"Fast", 50, 25, 25},
  {"Normal", 80, 40, 40},
  {"Safe", 120, 60, 60},
  {"Remote Play", 200, 80, 80},
};
const size_t SPEED_COUNT = sizeof(SPEEDS) / sizeof(SPEEDS[0]);

// Chat: key that opens the chat box (Enter by default in Helldivers 2 on PC)
const KeyOption CHAT_KEYS[] = {{"Enter", K_ENTER}, {"T", K_T}, {"Y", K_Y}, {"None", 0}};
const size_t CHAT_KEY_COUNT = sizeof(CHAT_KEYS) / sizeof(CHAT_KEYS[0]);

static USBHIDKeyboard Keyboard;
static USBCDC *s_serial = nullptr;
static QueueHandle_t s_queue;
static volatile bool s_busy = false;
static volatile int s_step = -1;

void hid_log(const char *fmt, ...) {
  char buf[160];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  Serial.println(buf);  // UART0 header pins
  if (s_serial && *s_serial) s_serial->println(buf);
}

static bool wireless() { return g_settings.output == 1; }

// Resolves the current settings into a self-contained job (same for cable and dongle)
static void make_job(MacroJob &j, MacroKind kind, const char *data) {
  memset(&j, 0, sizeof(j));
  const SpeedOption &sp = SPEEDS[g_settings.speed < SPEED_COUNT ? g_settings.speed : 1];
  const DirSet &d = DIR_SETS[g_settings.dirSet < DIR_SET_COUNT ? g_settings.dirSet : 0];
  j.kind = kind;
  j.menuKey = MENU_KEYS[g_settings.menuKey < MENU_KEY_COUNT ? g_settings.menuKey : 0].key;
  j.menuHold = g_settings.menuMode == 0;
  j.chatKey = CHAT_KEYS[g_settings.chatKey < CHAT_KEY_COUNT ? g_settings.chatKey : 0].key;
  j.layout = g_settings.textLayout;
  j.up = d.up;
  j.down = d.down;
  j.left = d.left;
  j.right = d.right;
  j.openMs = sp.openMs;
  j.pressMs = sp.pressMs;
  j.gapMs = sp.gapMs;
  strncpy(j.data, data, MACRO_DATA_LEN);
}

static void on_step(int i) { s_step = i; }

static void hid_task(void *) {
  MacroJob job;
  for (;;) {
    if (xQueueReceive(s_queue, &job, portMAX_DELAY) != pdTRUE) continue;
    macro_run(Keyboard, job, on_step);
    s_step = -1;
    s_busy = false;
  }
}

void hid_begin(bool withSerial) {
  if (withSerial) s_serial = new USBCDC(0);  // must exist before USB.begin()
  Keyboard.begin();
  USB.begin();
  if (s_serial) s_serial->begin(115200);
  s_queue = xQueueCreate(1, sizeof(MacroJob));
  xTaskCreatePinnedToCore(hid_task, "hid", 4096, nullptr, 5, nullptr, 1);  // core 1: Wi-Fi runs on core 0
}

bool hid_connected() { return wireless() ? link_connected() : (bool)USB; }
const char *hid_output_name() { return wireless() ? "Dongle" : "USB"; }

static bool enqueue(MacroKind kind, const char *data) {
  if (hid_busy()) return false;
  MacroJob job;
  make_job(job, kind, data);
  if (wireless()) return link_send(job);
  s_busy = true;
  if (xQueueSend(s_queue, &job, 0) != pdTRUE) {
    s_busy = false;
    return false;
  }
  return true;
}

bool hid_send(const char *code) { return enqueue(MJ_STRAT, code); }
bool hid_send_text(const char *text) { return enqueue(MJ_TEXT, text); }

Stream *hid_serial() { return s_serial; }
bool hid_busy() { return s_busy || link_busy(); }
int hid_step() { return wireless() ? link_step() : s_step; }
