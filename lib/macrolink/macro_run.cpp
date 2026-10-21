#include "macrolink.h"
#include <Arduino.h>
#include <USBHIDKeyboard.h>

#define K_ENTER 0x28

const MacroLayout MACRO_LAYOUTS[] = {
  {"US QWERTY", KeyboardLayout_en_US}, {"FR AZERTY", KeyboardLayout_fr_FR}, {"DE QWERTZ", KeyboardLayout_de_DE},
  {"ES", KeyboardLayout_es_ES}, {"IT", KeyboardLayout_it_IT},
};
const size_t MACRO_LAYOUT_COUNT = sizeof(MACRO_LAYOUTS) / sizeof(MACRO_LAYOUTS[0]);

static void tap(USBHIDKeyboard &kb, uint8_t key, uint16_t pressMs, uint16_t gapMs) {
  kb.pressRaw(key);
  vTaskDelay(pdMS_TO_TICKS(pressMs));
  kb.releaseRaw(key);
  vTaskDelay(pdMS_TO_TICKS(gapMs));
}

static uint8_t dir_key(const MacroJob &j, char c) {
  switch (c) {
    case 'U': return j.up;
    case 'D': return j.down;
    case 'L': return j.left;
    case 'R': return j.right;
  }
  return 0;
}

static void run_text(USBHIDKeyboard &kb, const MacroJob &j, void (*onStep)(int)) {
  kb.begin(MACRO_LAYOUTS[j.layout < MACRO_LAYOUT_COUNT ? j.layout : 0].map);  // only swaps the ASCII map
  uint16_t pressMs = j.pressMs < 20 ? 20 : j.pressMs;
  uint16_t gapMs = j.gapMs / 2 < 15 ? 15 : j.gapMs / 2;

  if (j.chatKey) {
    tap(kb, j.chatKey, pressMs, 0);
    vTaskDelay(pdMS_TO_TICKS(j.openMs * 2));  // chat box needs a moment to take focus
  }
  for (int i = 0; j.data[i]; i++) {
    onStep(i);
    kb.press((uint8_t)j.data[i]);  // layout-aware, adds Shift/AltGr as needed
    vTaskDelay(pdMS_TO_TICKS(pressMs));
    kb.releaseAll();
    vTaskDelay(pdMS_TO_TICKS(gapMs));
  }
  vTaskDelay(pdMS_TO_TICKS(j.openMs));
  tap(kb, K_ENTER, pressMs, 0);
}

static void run_strat(USBHIDKeyboard &kb, const MacroJob &j, void (*onStep)(int)) {
  if (j.menuHold) kb.pressRaw(j.menuKey);
  else tap(kb, j.menuKey, j.pressMs, 0);
  vTaskDelay(pdMS_TO_TICKS(j.openMs));

  for (int i = 0; j.data[i]; i++) {
    uint8_t k = dir_key(j, j.data[i]);
    if (!k) continue;
    onStep(i);
    tap(kb, k, j.pressMs, j.gapMs);
  }
  if (j.menuHold) {
    vTaskDelay(pdMS_TO_TICKS(j.gapMs));
    kb.releaseRaw(j.menuKey);
  }
}

void macro_run(USBHIDKeyboard &kb, const MacroJob &job, void (*onStep)(int)) {
  MacroJob j = job;
  j.data[MACRO_DATA_LEN] = 0;
  kb.releaseAll();
  if (j.kind == MJ_TEXT) run_text(kb, j, onStep);
  else run_strat(kb, j, onStep);
  kb.releaseAll();
}
