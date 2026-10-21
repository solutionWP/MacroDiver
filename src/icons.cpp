#include "icons.h"
#include "icons_index.h"

// data/icons.bin is linked into flash via board_build.embed_files
extern const uint8_t icons_bin_start[] asm("_binary_data_icons_bin_start");

static lv_img_dsc_t s_dsc[ICON_COUNT][2];
static bool s_ready = false;

static void init() {
  for (int i = 0; i < ICON_COUNT; i++) {
    for (int k = 0; k < 2; k++) {
      int sz = k == 0 ? ICON_LARGE : ICON_SMALL;
      lv_img_dsc_t &d = s_dsc[i][k];
      d.header.always_zero = 0;
      d.header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA;
      d.header.w = sz;
      d.header.h = sz;
      d.data_size = sz * sz * LV_IMG_PX_SIZE_ALPHA_BYTE;
      d.data = icons_bin_start + (k == 0 ? ICON_INDEX[i].offLarge : ICON_INDEX[i].offSmall);
    }
  }
  s_ready = true;
}

int icon_count() { return ICON_COUNT; }
int icon_small_size() { return ICON_SMALL; }
const uint8_t *icon_small_data(int index) { return icons_bin_start + ICON_INDEX[index].offSmall; }
int icon_index(uint16_t id) {
  for (int i = 0; i < ICON_COUNT; i++)
    if (ICON_INDEX[i].id == id) return i;
  return -1;
}

const lv_img_dsc_t *icon_get(uint16_t id, bool large) {
  if (!s_ready) init();
  for (int i = 0; i < ICON_COUNT; i++)
    if (ICON_INDEX[i].id == id) return &s_dsc[i][large ? 0 : 1];
  return nullptr;
}
