#pragma once
#include <stdint.h>
#include <stddef.h>

// Stratagem categories (in-game colour groups: offensive red, supply blue, defensive green, mission yellow)
enum StratCat : uint8_t {
  CAT_ORBITAL = 0,
  CAT_EAGLE,
  CAT_SUPPORT,
  CAT_BACKPACK,
  CAT_VEHICLE,
  CAT_SENTRY,
  CAT_EMPLACEMENT,
  CAT_MISSION,
  CAT_CUSTOM,
  CAT_MESSAGE,  // typed chat text, not an arrow code
  CAT_COUNT
};

struct Strat {
  uint16_t id;           // stable ID - saved in builds, never reuse/renumber
  uint8_t cat;
  const char *name;      // full in-game name
  const char *shortName; // tile label
  const char *code;      // U/D/L/R sequence
};

#define CUSTOM_ID_BASE 1000
#define MSG_ID_BASE    2000

const char *cat_name(uint8_t cat);
uint32_t cat_color(uint8_t cat);

// Built-in catalog
size_t strat_catalog_count();
const Strat *strat_catalog_at(size_t i);

// Lookup by ID (built-in or custom). Returns nullptr if unknown.
const Strat *strat_get(uint16_t id);

// Converts "UDLR" to an LVGL arrow-symbol string. out must hold 4*strlen(code)+1 bytes.
void strat_code_to_arrows(const char *code, char *out, size_t outLen);
