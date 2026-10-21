#include "stratagems.h"
#include "storage.h"
#include <string.h>
#include <lvgl.h>

// Codes from helldivers.wiki.gg (October 2026). U=Up D=Down L=Left R=Right.
// IDs are stable: append new entries with new IDs, never renumber.
static const Strat CATALOG[] = {
  // ---- Orbital ----
  {  1, CAT_ORBITAL, "Orbital Precision Strike",  "Precision Strike", "RRU"},
  {  2, CAT_ORBITAL, "Orbital Gatling Barrage",   "Gatling Barrage",  "RDLUU"},
  {  3, CAT_ORBITAL, "Orbital Gas Strike",        "Orbital Gas",      "RRDR"},
  {  4, CAT_ORBITAL, "Orbital 120mm HE Barrage",  "120mm Barrage",    "RRDLRD"},
  {  5, CAT_ORBITAL, "Orbital Airburst Strike",   "Airburst Strike",  "RRR"},
  {  6, CAT_ORBITAL, "Orbital Smoke Strike",      "Orbital Smoke",    "RRDU"},
  {  7, CAT_ORBITAL, "Orbital EMS Strike",        "Orbital EMS",      "RRLD"},
  {  8, CAT_ORBITAL, "Orbital 380mm HE Barrage",  "380mm Barrage",    "RDUULDD"},
  {  9, CAT_ORBITAL, "Orbital Walking Barrage",   "Walking Barrage",  "RDRDRD"},
  { 10, CAT_ORBITAL, "Orbital Laser",             "Orbital Laser",    "RDURD"},
  { 11, CAT_ORBITAL, "Orbital Napalm Barrage",    "Napalm Barrage",   "RRDLRU"},
  { 12, CAT_ORBITAL, "Orbital Railcannon Strike", "Railcannon",       "RUDDR"},
  // ---- Eagle ----
  { 20, CAT_EAGLE, "Eagle Strafing Run",      "Strafing Run",    "URR"},
  { 21, CAT_EAGLE, "Eagle Airstrike",         "Eagle Airstrike", "URDR"},
  { 22, CAT_EAGLE, "Eagle Cluster Bomb",      "Cluster Bomb",    "URDDR"},
  { 23, CAT_EAGLE, "Eagle Napalm Airstrike",  "Eagle Napalm",    "URDU"},
  { 24, CAT_EAGLE, "Eagle Smoke Strike",      "Eagle Smoke",     "URUD"},
  { 25, CAT_EAGLE, "Eagle 110mm Rocket Pods", "Rocket Pods",     "URUL"},
  { 26, CAT_EAGLE, "Eagle 500kg Bomb",        "500kg Bomb",      "URDDD"},
  { 27, CAT_EAGLE, "Eagle Gas Airstrike",     "Eagle Gas",       "URLR"},
  // ---- Support weapons ----
  { 40, CAT_SUPPORT, "MG-43 Machine Gun",               "Machine Gun",      "DLDUR"},
  { 41, CAT_SUPPORT, "EAT-17 Expendable Anti-Tank",     "EAT-17",           "DDLUR"},
  { 42, CAT_SUPPORT, "M-105 Stalwart",                  "Stalwart",         "DLDUUL"},
  { 43, CAT_SUPPORT, "LAS-98 Laser Cannon",             "Laser Cannon",     "DLDUL"},
  { 44, CAT_SUPPORT, "APW-1 Anti-Materiel Rifle",       "Anti-Materiel",    "DLRUD"},
  { 45, CAT_SUPPORT, "GL-21 Grenade Launcher",          "Grenade Launcher", "DLULD"},
  { 46, CAT_SUPPORT, "GR-8 Recoilless Rifle",           "Recoilless",       "DLRRL"},
  { 47, CAT_SUPPORT, "FLAM-40 Flamethrower",            "Flamethrower",     "DLUDU"},
  { 48, CAT_SUPPORT, "MG-206 Heavy Machine Gun",        "Heavy MG",         "DLUDD"},
  { 49, CAT_SUPPORT, "AC-8 Autocannon",                 "Autocannon",       "DLDUUR"},
  { 50, CAT_SUPPORT, "ARC-3 Arc Thrower",               "Arc Thrower",      "DRDULL"},
  { 51, CAT_SUPPORT, "LAS-99 Quasar Cannon",            "Quasar Cannon",    "DDULR"},
  { 52, CAT_SUPPORT, "RL-77 Airburst Rocket Launcher",  "Airburst RL",      "DUULR"},
  { 53, CAT_SUPPORT, "MLS-4X Commando",                 "Commando",         "DLUDR"},
  { 54, CAT_SUPPORT, "FAF-14 Spear",                    "Spear",            "DDUDD"},
  { 55, CAT_SUPPORT, "RS-422 Railgun",                  "Railgun",          "DRDULR"},
  { 56, CAT_SUPPORT, "StA-X3 W.A.S.P. Launcher",        "W.A.S.P.",         "DDUDR"},
  { 57, CAT_SUPPORT, "CQC-20 Breaching Hammer",         "Breaching Hammer", "DLRLU"},
  { 58, CAT_SUPPORT, "PLAS-45 Epoch",                   "Epoch",            "DLULR"},
  { 59, CAT_SUPPORT, "MGX-42 Bullet Storm",             "Bullet Storm",     "DLDRUL"},
  { 60, CAT_SUPPORT, "TX-41 Sterilizer",                "Sterilizer",       "DLUDL"},
  { 61, CAT_SUPPORT, "S-11 Speargun",                   "Speargun",         "DRDLUR"},
  { 62, CAT_SUPPORT, "CQC-9 Defoliation Tool",          "Defoliation Tool", "DLRRD"},
  { 63, CAT_SUPPORT, "GL-52 De-Escalator",              "De-Escalator",     "DRULR"},
  { 64, CAT_SUPPORT, "EAT-700 Expendable Napalm",       "EAT Napalm",       "DDLUL"},
  { 65, CAT_SUPPORT, "EAT-411 Leveller",                "Leveller",         "DDLUD"},
  { 66, CAT_SUPPORT, "M-1000 Maxigun",                  "Maxigun",          "DLRDUU"},
  { 67, CAT_SUPPORT, "GL-28 Belt-Fed Grenade Launcher", "Belt-Fed GL",      "DLULUU"},
  { 68, CAT_SUPPORT, "B/FLAM-80 Cremator",              "Cremator",         "DDRDUU"},
  { 69, CAT_SUPPORT, "MS-11 Solo Silo",                 "Solo Silo",        "DURDD"},
  { 70, CAT_SUPPORT, "40-K Meltagun",                   "Meltagun",         "DLULLD"},
  { 71, CAT_SUPPORT, "B/MD C4 Pack",                    "C4 Pack",          "DRUURU"},
  { 72, CAT_SUPPORT, "CQC-1 One True Flag",             "One True Flag",    "DLRRU"},
  // ---- Backpacks ----
  {100, CAT_BACKPACK, "B-1 Supply Pack",                "Supply Pack",       "DLDUUD"},
  {101, CAT_BACKPACK, "LIFT-850 Jump Pack",             "Jump Pack",         "DUUDU"},
  {102, CAT_BACKPACK, "SH-20 Ballistic Shield Backpack","Ballistic Shield",  "DLDDUL"},
  {103, CAT_BACKPACK, "AX/AR-23 Guard Dog",             "Guard Dog",         "DULURD"},
  {104, CAT_BACKPACK, "AX/LAS-5 Rover",                 "Rover",             "DULURR"},
  {105, CAT_BACKPACK, "SH-32 Shield Generator Pack",    "Shield Pack",       "DULRLR"},
  {106, CAT_BACKPACK, "SH-51 Directional Shield",       "Directional Shield","DULRUU"},
  {107, CAT_BACKPACK, "AX/FLAM-75 Hot Dog",             "Hot Dog",           "DULULL"},
  {108, CAT_BACKPACK, "B-100 Portable Hellbomb",        "Portable Hellbomb", "DRUUU"},
  {109, CAT_BACKPACK, "AX/ARC-3 K-9",                   "K-9",               "DULURL"},
  {110, CAT_BACKPACK, "LIFT-860 Hover Pack",            "Hover Pack",        "DUUDLR"},
  {111, CAT_BACKPACK, "AX/TX-13 Dog Breath",            "Dog Breath",        "DULURU"},
  {112, CAT_BACKPACK, "LIFT-182 Warp Pack",             "Warp Pack",         "DLRDLR"},
  // ---- Vehicles ----
  {130, CAT_VEHICLE, "M-102 Gunner FRV",            "Gunner FRV",      "LDRDRDU"},
  {131, CAT_VEHICLE, "M-103 Supply FRV",            "Supply FRV",      "LDLLDUR"},
  {132, CAT_VEHICLE, "M-104 Incinerator FRV",       "Incinerator FRV", "LDRLDUU"},
  {133, CAT_VEHICLE, "EXO-45 Patriot Exosuit",      "Patriot Exosuit", "LDRULDD"},
  {134, CAT_VEHICLE, "EXO-49 Emancipator Exosuit",  "Emancipator",     "LDRULDU"},
  {135, CAT_VEHICLE, "EXO-55 Breakthrough Exosuit", "Breakthrough",    "LDRLRDU"},
  {136, CAT_VEHICLE, "EXO-51 Lumberer Exosuit",     "Lumberer",        "LDRURLU"},
  {137, CAT_VEHICLE, "TD-110 Maelstrom",            "Maelstrom",       "LDRDLDULR"},
  {138, CAT_VEHICLE, "TD-220 Bastion MK XVI",       "Bastion",         "LDRDLDUDU"},
  // ---- Sentries ----
  {150, CAT_SENTRY, "A/MG-43 Machine Gun Sentry",  "MG Sentry",         "DURRU"},
  {151, CAT_SENTRY, "A/G-16 Gatling Sentry",       "Gatling Sentry",    "DURL"},
  {152, CAT_SENTRY, "A/M-12 Mortar Sentry",        "Mortar Sentry",     "DURRD"},
  {153, CAT_SENTRY, "A/AC-8 Autocannon Sentry",    "Autocannon Sentry", "DURULU"},
  {154, CAT_SENTRY, "A/MLS-4X Rocket Sentry",      "Rocket Sentry",     "DURRL"},
  {155, CAT_SENTRY, "A/M-23 EMS Mortar Sentry",    "EMS Mortar",        "DURDR"},
  {156, CAT_SENTRY, "A/ARC-3 Tesla Tower",         "Tesla Tower",       "DURULR"},
  {157, CAT_SENTRY, "A/LAS-98 Laser Sentry",       "Laser Sentry",      "DURDUR"},
  {158, CAT_SENTRY, "A/FLAM-40 Flame Sentry",      "Flame Sentry",      "DURDUU"},
  {159, CAT_SENTRY, "A/GM-17 Gas Mortar Sentry",   "Gas Mortar",        "DURDL"},
  // ---- Emplacements ----
  {170, CAT_EMPLACEMENT, "MD-6 Anti-Personnel Minefield", "AP Mines",         "DLUR"},
  {171, CAT_EMPLACEMENT, "MD-I4 Incendiary Mines",        "Incendiary Mines", "DLLD"},
  {172, CAT_EMPLACEMENT, "MD-17 Anti-Tank Mines",         "AT Mines",         "DLUU"},
  {173, CAT_EMPLACEMENT, "MD-8 Gas Mines",                "Gas Mines",        "DLLR"},
  {174, CAT_EMPLACEMENT, "FX-12 Shield Generator Relay",  "Shield Relay",     "DDLRLR"},
  {175, CAT_EMPLACEMENT, "E/MG-101 HMG Emplacement",      "HMG Emplacement",  "DULRRL"},
  {176, CAT_EMPLACEMENT, "E/GL-21 Grenadier Battlement",  "Grenadier Battlement", "DRDLR"},
  {177, CAT_EMPLACEMENT, "E/AT-12 Anti-Tank Emplacement", "AT Emplacement",   "DULRRR"},
  // ---- Mission / general ----
  {200, CAT_MISSION, "Reinforce",                       "Reinforce",       "UDRLU"},
  {201, CAT_MISSION, "Resupply",                        "Resupply",        "DDUR"},
  {202, CAT_MISSION, "SoS Beacon",                      "SoS Beacon",      "UDRU"},
  {203, CAT_MISSION, "NUX-223 Hellbomb",                "Hellbomb",        "DULDURDU"},
  {204, CAT_MISSION, "Eagle Rearm",                     "Eagle Rearm",     "UULUR"},
  {205, CAT_MISSION, "SEAF Artillery",                  "SEAF Artillery",  "RUUD"},
  {206, CAT_MISSION, "Super Earth Flag",                "SE Flag",         "DUDU"},
  {207, CAT_MISSION, "SSSD Delivery",                   "SSSD Delivery",   "DDDUU"},
  {208, CAT_MISSION, "Upload Data",                     "Upload Data",     "LRUUU"},
  {209, CAT_MISSION, "Seismic Probe",                   "Seismic Probe",   "UULRDD"},
  {210, CAT_MISSION, "Hive Breaker Drill",              "Hive Breaker",    "LUDRDD"},
  {211, CAT_MISSION, "Prospecting Drill",               "Prospecting Drill","DDLRDD"},
  {212, CAT_MISSION, "Dark Fluid Vessel",               "Dark Fluid",      "ULRDUU"},
  {213, CAT_MISSION, "Tectonic Drill",                  "Tectonic Drill",  "UDUDUD"},
  {214, CAT_MISSION, "Call In Super Destroyer",         "Super Destroyer", "UUDDLRLR"},
  {215, CAT_MISSION, "Portable Comms Relay",            "Comms Relay",     "UUDDLLD"},
  {216, CAT_MISSION, "Tactical Video Camera",           "Video Camera",    "RDDULLU"},
  {217, CAT_MISSION, "Cargo Container",                 "Cargo Container", "UUDDRD"},
  {218, CAT_MISSION, "Activate E-711 Extraction Drill", "E-711 Drill",     "DDLLDD"},
  {219, CAT_MISSION, "Aquifer Drill",                   "Aquifer Drill",   "LLLUDRDD"},
  {220, CAT_MISSION, "Reinforcement Pods",              "Reinf. Pods",     "LRUUU"},
};

static const size_t CATALOG_N = sizeof(CATALOG) / sizeof(CATALOG[0]);

static const char *CAT_NAMES[CAT_COUNT] = {
  "Orbital", "Eagle", "Support Weapons", "Backpacks", "Vehicles",
  "Sentries", "Emplacements", "Mission", "Custom", "Messages"};

static const uint32_t CAT_COLORS[CAT_COUNT] = {
  0xE5533D, 0xE5533D,            // offensive: red
  0x49A6E9, 0x49A6E9, 0x49A6E9,  // supply: blue
  0x66C15A, 0x66C15A,            // defensive: green
  0xF5C542,                      // mission: yellow
  0xB07CF2,                      // custom: purple
  0x4FD1C5};                     // messages: teal

const char *cat_name(uint8_t cat) { return cat < CAT_COUNT ? CAT_NAMES[cat] : "?"; }
uint32_t cat_color(uint8_t cat) { return cat < CAT_COUNT ? CAT_COLORS[cat] : 0xFFFFFF; }

size_t strat_catalog_count() { return CATALOG_N; }
const Strat *strat_catalog_at(size_t i) { return i < CATALOG_N ? &CATALOG[i] : nullptr; }

const Strat *strat_get(uint16_t id) {
  if (id >= MSG_ID_BASE) return msg_find(id);
  if (id >= CUSTOM_ID_BASE) return custom_find(id);
  for (size_t i = 0; i < CATALOG_N; i++)
    if (CATALOG[i].id == id) return &CATALOG[i];
  return nullptr;
}

void strat_code_to_arrows(const char *code, char *out, size_t outLen) {
  size_t n = 0;
  out[0] = 0;
  for (const char *c = code; *c; c++) {
    const char *sym = nullptr;
    switch (*c) {
      case 'U': sym = LV_SYMBOL_UP; break;
      case 'D': sym = LV_SYMBOL_DOWN; break;
      case 'L': sym = LV_SYMBOL_LEFT; break;
      case 'R': sym = LV_SYMBOL_RIGHT; break;
    }
    if (!sym) continue;
    size_t l = strlen(sym);
    if (n + l + 1 >= outLen) break;
    memcpy(out + n, sym, l);
    n += l;
    out[n] = 0;
  }
}
