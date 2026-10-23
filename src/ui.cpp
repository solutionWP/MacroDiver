// MacroDiver UI - LVGL 8.3, 412x412 round display.
// Everything interactive is kept inside the visible circle (radius 206) and
// touch targets are separated by >= 12 px (lists) / >= 26 px (pad tiles).
#include "ui.h"
#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <string.h>
#include "display.h"
#include "storage.h"
#include "stratagems.h"
#include "hid_out.h"
#include "link.h"
#include "icons.h"
#include "web.h"

#define COL_BG      0x000000
#define COL_ROW     0x1C2026
#define COL_ROW_PR  0x323A46
#define COL_ROW_CK  0x3A3410
#define COL_TILE    0x14171B
#define COL_YELLOW  0xFFE710
#define COL_DIM     0x8A929C
#define COL_RED     0xE5533D
#define COL_GREEN   0x66C15A

LV_FONT_DECLARE(lv_font_montserrat_12);
LV_FONT_DECLARE(lv_font_montserrat_14);
LV_FONT_DECLARE(lv_font_montserrat_16);
LV_FONT_DECLARE(lv_font_montserrat_20);
LV_FONT_DECLARE(lv_font_montserrat_24);
LV_FONT_DECLARE(lv_font_montserrat_28);

// ---------------------------------------------------------------- styles
static lv_style_t st_row, st_row_pr, st_row_ck, st_list, st_btn, st_btn_pr, st_tile, st_tile_ck;

static void styles_init() {
  lv_style_init(&st_row);
  lv_style_set_bg_color(&st_row, lv_color_hex(COL_ROW));
  lv_style_set_bg_opa(&st_row, LV_OPA_COVER);
  lv_style_set_radius(&st_row, 12);
  lv_style_set_border_side(&st_row, LV_BORDER_SIDE_LEFT);
  lv_style_set_border_width(&st_row, 6);
  lv_style_set_border_color(&st_row, lv_color_hex(COL_DIM));
  lv_style_set_pad_left(&st_row, 16);
  lv_style_set_pad_right(&st_row, 10);
  lv_style_set_text_color(&st_row, lv_color_white());
  lv_style_set_text_font(&st_row, &lv_font_montserrat_20);

  lv_style_init(&st_row_pr);
  lv_style_set_bg_color(&st_row_pr, lv_color_hex(COL_ROW_PR));

  lv_style_init(&st_row_ck);
  lv_style_set_bg_color(&st_row_ck, lv_color_hex(COL_ROW_CK));
  lv_style_set_border_side(&st_row_ck, LV_BORDER_SIDE_FULL);
  lv_style_set_border_width(&st_row_ck, 3);
  lv_style_set_border_color(&st_row_ck, lv_color_hex(COL_YELLOW));

  lv_style_init(&st_list);
  lv_style_set_pad_row(&st_list, 12);  // clearance between rows
  lv_style_set_pad_ver(&st_list, 6);

  lv_style_init(&st_btn);
  lv_style_set_bg_color(&st_btn, lv_color_hex(COL_ROW));
  lv_style_set_bg_opa(&st_btn, LV_OPA_COVER);
  lv_style_set_radius(&st_btn, 22);
  lv_style_set_border_width(&st_btn, 2);
  lv_style_set_border_color(&st_btn, lv_color_hex(COL_YELLOW));
  lv_style_set_text_color(&st_btn, lv_color_hex(COL_YELLOW));
  lv_style_set_text_font(&st_btn, &lv_font_montserrat_20);

  lv_style_init(&st_btn_pr);
  lv_style_set_bg_color(&st_btn_pr, lv_color_hex(COL_YELLOW));
  lv_style_set_text_color(&st_btn_pr, lv_color_black());

  lv_style_init(&st_tile);
  lv_style_set_bg_color(&st_tile, lv_color_hex(COL_TILE));
  lv_style_set_bg_opa(&st_tile, LV_OPA_COVER);
  lv_style_set_radius(&st_tile, LV_RADIUS_CIRCLE);
  lv_style_set_border_width(&st_tile, 4);
  lv_style_set_text_color(&st_tile, lv_color_white());

  lv_style_init(&st_tile_ck);  // "sending" state
  lv_style_set_border_color(&st_tile_ck, lv_color_white());
  lv_style_set_border_width(&st_tile_ck, 6);
}

// ---------------------------------------------------------------- helpers
static lv_timer_t *s_homeTimer = nullptr;
static lv_timer_t *s_screenTimer = nullptr;  // owned by the current non-home screen
static bool s_onHome = false;
static bool s_onWeb = false;

// LVGL's default PRESS_LOCK fires CLICKED even if the finger slid off the
// object before release. Clear it so sliding away cancels the press.
static void no_press_lock(lv_obj_t *o) { lv_obj_clear_flag(o, LV_OBJ_FLAG_PRESS_LOCK); }

static lv_obj_t *new_screen() {
  lv_obj_t *s = lv_obj_create(NULL);
  lv_obj_remove_style_all(s);
  lv_obj_set_size(s, SCREEN_W, SCREEN_H);
  lv_obj_set_style_bg_color(s, lv_color_hex(COL_BG), 0);
  lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
  lv_obj_clear_flag(s, LV_OBJ_FLAG_SCROLLABLE);
  return s;
}

static void go(lv_obj_t *scr) {
  if (s_homeTimer) {
    lv_timer_del(s_homeTimer);
    s_homeTimer = nullptr;
  }
  if (s_screenTimer) {
    lv_timer_del(s_screenTimer);
    s_screenTimer = nullptr;
  }
  s_onWeb = false;
  s_onHome = false;
  lv_obj_t *old = lv_scr_act();
  lv_scr_load(scr);
  if (old && old != scr) lv_obj_del_async(old);  // we may be inside one of its events
}

static lv_obj_t *add_label(lv_obj_t *parent, const char *txt, const lv_font_t *font, uint32_t color) {
  lv_obj_t *l = lv_label_create(parent);
  lv_label_set_text(l, txt);
  lv_obj_set_style_text_font(l, font, 0);
  lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
  return l;
}

static lv_obj_t *add_title(lv_obj_t *scr, const char *txt) {
  lv_obj_t *t = add_label(scr, txt, &lv_font_montserrat_20, COL_YELLOW);
  lv_label_set_long_mode(t, LV_LABEL_LONG_DOT);
  lv_obj_set_width(t, 230);
  lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 26);
  return t;
}

static lv_obj_t *add_button(lv_obj_t *parent, const char *txt, int w, int h, lv_event_cb_t cb, void *ud) {
  lv_obj_t *b = lv_btn_create(parent);
  lv_obj_remove_style_all(b);
  lv_obj_add_style(b, &st_btn, 0);
  lv_obj_add_style(b, &st_btn_pr, LV_STATE_PRESSED);
  lv_obj_set_size(b, w, h);
  no_press_lock(b);
  lv_obj_t *l = lv_label_create(b);
  lv_label_set_text(l, txt);
  lv_obj_center(l);
  if (cb) lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, ud);
  return b;
}

// Bottom-centre BACK button (kept inside the circle)
static lv_obj_t *add_back(lv_obj_t *scr, lv_event_cb_t cb, void *ud = nullptr) {
  lv_obj_t *b = add_button(scr, LV_SYMBOL_LEFT "  Back", 130, 46, cb, ud);
  lv_obj_align(b, LV_ALIGN_BOTTOM_MID, 0, -14);
  return b;
}

// Scrollable column centred on screen between title and back button
static lv_obj_t *add_list(lv_obj_t *scr, int top = 62, int height = 274, int width = 296) {
  lv_obj_t *l = lv_obj_create(scr);
  lv_obj_remove_style_all(l);
  lv_obj_add_style(l, &st_list, 0);
  lv_obj_set_size(l, width, height);
  lv_obj_align(l, LV_ALIGN_TOP_MID, 0, top);
  lv_obj_set_flex_flow(l, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_scrollbar_mode(l, LV_SCROLLBAR_MODE_OFF);
  lv_obj_set_scroll_dir(l, LV_DIR_VER);
  return l;
}

static lv_obj_t *add_row(lv_obj_t *list, const char *txt, lv_event_cb_t cb, void *ud, uint32_t accent = COL_DIM, int h = 54) {
  lv_obj_t *r = lv_btn_create(list);
  lv_obj_remove_style_all(r);
  lv_obj_add_style(r, &st_row, 0);
  lv_obj_add_style(r, &st_row_pr, LV_STATE_PRESSED);
  lv_obj_add_style(r, &st_row_ck, LV_STATE_CHECKED);
  lv_obj_set_style_border_color(r, lv_color_hex(accent), 0);
  lv_obj_set_size(r, LV_PCT(100), h);
  no_press_lock(r);
  lv_obj_t *l = lv_label_create(r);
  lv_label_set_text(l, txt);
  lv_label_set_long_mode(l, LV_LABEL_LONG_DOT);
  lv_obj_set_width(l, LV_PCT(100));
  lv_obj_align(l, LV_ALIGN_LEFT_MID, 0, 0);
  if (cb) lv_obj_add_event_cb(r, cb, LV_EVENT_CLICKED, ud);
  return r;
}

static void row_set_text(lv_obj_t *row, const char *txt) { lv_label_set_text(lv_obj_get_child(row, 0), txt); }

#define UD(i) ((void *)(intptr_t)(i))
#define EV_INT(e) ((int)(intptr_t)lv_event_get_user_data(e))

// Forward declarations
static void show_home();
static void show_menu();
static void show_builds();
static void show_build_opts(int bi);
static void show_edit(int bi);
static void show_categories(int bi);
static void show_picker(int bi, int cat);
static void show_settings();
static void show_customs();
static void show_custom_opts(int j);
static void show_code_editor(int j);
static void show_msgs();
static void show_msg_opts(int j);
static void show_web();
typedef void (*TextDoneCb)(const char *txt);
static void show_text_input(const char *title, const char *initial, TextDoneCb ok, void (*cancel)(), int maxLen = NAME_LEN);

// ======================================================================
// HOME / PAD
// ======================================================================
struct TilePos { int16_t x, y; };
// Large: 4 tiles, d=150, centres +-90 -> gap 30 px, outer edge r=202
static const TilePos POS_LARGE[4] = {{-90, -90}, {90, -90}, {-90, 90}, {90, 90}};
// Dense: hex 2-3-2, d=116, centre spacing 142 -> gap 26 px, outer edge r=200
static const TilePos POS_DENSE[7] = {{-71, -123}, {71, -123}, {-142, 0}, {0, 0}, {142, 0}, {-71, 123}, {71, 123}};

static lv_obj_t *s_homeTitle = nullptr, *s_tv = nullptr, *s_sendingTile = nullptr;
static int s_pages = 1;
static uint32_t s_msgUntil = 0;
static int s_homePage = 0;

static void home_update_title() {
  if (!s_homeTitle) return;
  if (millis() < s_msgUntil) return;
  Build *b = build_active();
  char usb[24];  // output icon: cable or dongle, green when it can type
  snprintf(usb, sizeof(usb), "#%s %s#", hid_connected() ? "66C15A" : "E5533D",
           g_settings.output == 1 ? LV_SYMBOL_WIFI : LV_SYMBOL_USB);
  if (s_pages > 1) lv_label_set_text_fmt(s_homeTitle, "%s %s  %d/%d", usb, b->name, s_homePage + 1, s_pages);
  else lv_label_set_text_fmt(s_homeTitle, "%s %s", usb, b->name);
}

static void home_flash(const char *msg, uint32_t color, uint32_t ms) {
  if (!s_homeTitle) return;
  lv_label_set_text_fmt(s_homeTitle, "#%06lX %s#", (unsigned long)color, msg);
  s_msgUntil = millis() + ms;
}

static void tile_colors(lv_obj_t *tile, bool pressed) {
  const Strat *s = strat_get((uint16_t)(intptr_t)lv_obj_get_user_data(tile));
  lv_color_t c = lv_color_hex(s ? cat_color(s->cat) : COL_DIM);
  lv_obj_set_style_bg_color(tile, pressed ? lv_color_mix(c, lv_color_hex(COL_TILE), 110) : lv_color_hex(COL_TILE), 0);
}

static void tile_event(lv_event_t *e) {
  lv_obj_t *tile = lv_event_get_target(e);
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_PRESSED) {
    tile_colors(tile, true);
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    tile_colors(tile, false);
  } else if (code == LV_EVENT_CLICKED) {  // released inside the tile, not a swipe
    const Strat *s = strat_get((uint16_t)(intptr_t)lv_obj_get_user_data(tile));
    if (!s) return;
    if (!hid_connected()) {
      char buf[32];
      snprintf(buf, sizeof(buf), "%s not connected", hid_output_name());
      home_flash(buf, COL_RED, 1500);
      return;
    }
    bool msg = s->cat == CAT_MESSAGE;
    if (!msg && !s->code[0]) {  // imported before its code was known
      home_flash("No code yet", COL_RED, 2000);
      return;
    }
    if (msg ? hid_send_text(s->name) : hid_send(s->code)) {
      s_sendingTile = tile;
      lv_obj_add_state(tile, LV_STATE_CHECKED);
      home_flash(s->shortName, cat_color(s->cat), 900);
      hid_log("Send %s (%s)", s->name, msg ? "chat" : s->code);
    }
  }
}

static void home_timer_cb(lv_timer_t *) {
  if (s_sendingTile && !hid_busy()) {
    lv_obj_clear_state(s_sendingTile, LV_STATE_CHECKED);
    s_sendingTile = nullptr;
  }
  home_update_title();
}

static void tv_event(lv_event_t *e) {
  lv_obj_t *act = lv_tileview_get_tile_act(s_tv);
  s_homePage = lv_obj_get_x(act) / SCREEN_W;
  s_msgUntil = 0;
  home_update_title();
}

static void gear_event(lv_event_t *) { show_menu(); }

static lv_obj_t *make_tile(lv_obj_t *parent, const Strat *s, int d, int x, int y, bool large) {
  lv_obj_t *t = lv_btn_create(parent);
  lv_obj_remove_style_all(t);
  lv_obj_add_style(t, &st_tile, 0);
  lv_obj_add_style(t, &st_tile_ck, LV_STATE_CHECKED);
  lv_obj_set_style_border_color(t, lv_color_hex(cat_color(s->cat)), 0);
  lv_obj_set_size(t, d, d);
  lv_obj_align(t, LV_ALIGN_CENTER, x, y);
  no_press_lock(t);
  lv_obj_set_user_data(t, (void *)(intptr_t)s->id);
  lv_obj_set_flex_flow(t, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(t, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(t, 2, 0);

  bool msg = s->cat == CAT_MESSAGE;
  if (msg) add_label(t, LV_SYMBOL_KEYBOARD, &lv_font_montserrat_16, cat_color(CAT_MESSAGE));
  const lv_img_dsc_t *ic = icon_get(s->id, large);
  if (ic) {
    lv_obj_t *img = lv_img_create(t);
    lv_img_set_src(img, ic);
  }
  // With an icon the name is a small caption; without one it's the main content
  lv_obj_t *name = lv_label_create(t);
  lv_label_set_text(name, s->shortName);
  lv_label_set_long_mode(name, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(name, d * (ic ? (large ? 72 : 84) : 78) / 100);
  lv_obj_set_style_text_align(name, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_line_space(name, -2, 0);
  const lv_font_t *nf = ic ? (large ? &lv_font_montserrat_14 : &lv_font_montserrat_12)
                           : (large ? &lv_font_montserrat_20 : &lv_font_montserrat_16);
  lv_obj_set_style_text_font(name, nf, 0);
  if (msg) {  // up to 3 lines of text, then "..."
    lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
    lv_obj_set_height(name, lv_font_get_line_height(nf) * 3 - 4);
  }

  if (g_settings.showCodes && !msg && (large || !ic)) {
    char arrows[4 * MAX_CODE + 8];
    strat_code_to_arrows(s->code, arrows, sizeof(arrows));
    lv_obj_t *a = lv_label_create(t);
    lv_label_set_text(a, arrows);
    lv_label_set_long_mode(a, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(a, d * 76 / 100);
    lv_obj_set_style_text_align(a, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(a, (large && !ic) ? &lv_font_montserrat_20 : &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(a, lv_color_hex(cat_color(s->cat)), 0);
  }
  lv_obj_add_event_cb(t, tile_event, LV_EVENT_ALL, nullptr);
  return t;
}

static void show_home() {
  Build *b = build_active();
  lv_obj_t *scr = new_screen();
  bool large = b->layout == LAYOUT_LARGE;
  int per = large ? 4 : 7;
  int d = large ? 150 : 116;
  const TilePos *pos = large ? POS_LARGE : POS_DENSE;

  // Collect valid entries (skip IDs that no longer exist)
  const Strat *list[MAX_SLOTS];
  int n = 0;
  for (int k = 0; k < b->count; k++) {
    const Strat *s = strat_get(b->ids[k]);
    if (s) list[n++] = s;
  }
  s_pages = n == 0 ? 1 : (n + per - 1) / per;
  if (s_homePage >= s_pages) s_homePage = 0;

  s_tv = lv_tileview_create(scr);
  lv_obj_set_size(s_tv, SCREEN_W, SCREEN_H);
  lv_obj_set_style_bg_opa(s_tv, LV_OPA_TRANSP, 0);
  lv_obj_set_scrollbar_mode(s_tv, LV_SCROLLBAR_MODE_OFF);
  for (int p = 0; p < s_pages; p++) {
    lv_obj_t *page = lv_tileview_add_tile(s_tv, p, 0, LV_DIR_LEFT | LV_DIR_RIGHT);
    for (int k = 0; k < per; k++) {
      int idx = p * per + k;
      if (idx >= n) break;
      make_tile(page, list[idx], d, pos[k].x, pos[k].y, large);
    }
  }
  lv_obj_add_event_cb(s_tv, tv_event, LV_EVENT_VALUE_CHANGED, nullptr);

  if (n == 0) {
    lv_obj_t *l = add_label(scr, "Empty build\n\nPress BOOT or hold\n" LV_SYMBOL_SETTINGS " to add stratagems", &lv_font_montserrat_20, COL_DIM);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(l);
  }

  // Title - small, top centre, clear of the top tiles
  s_homeTitle = lv_label_create(scr);
  lv_label_set_recolor(s_homeTitle, true);
  lv_label_set_long_mode(s_homeTitle, LV_LABEL_LONG_DOT);
  lv_obj_set_width(s_homeTitle, 150);
  lv_obj_set_style_text_align(s_homeTitle, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(s_homeTitle, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_homeTitle, lv_color_hex(COL_YELLOW), 0);
  lv_obj_align(s_homeTitle, LV_ALIGN_TOP_MID, 0, 8);

  // Menu: long-press only, so a stray touch mid-fight can't open it
  lv_obj_t *gear = lv_btn_create(scr);
  lv_obj_remove_style_all(gear);
  lv_obj_set_size(gear, 40, 40);
  no_press_lock(gear);
  lv_obj_align(gear, LV_ALIGN_CENTER, 0, 184);
  lv_obj_set_style_radius(gear, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(gear, lv_color_hex(COL_ROW), 0);
  lv_obj_set_style_bg_opa(gear, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(gear, lv_color_hex(COL_YELLOW), LV_STATE_PRESSED);
  lv_obj_set_ext_click_area(gear, 0);
  lv_obj_t *gl = add_label(gear, LV_SYMBOL_SETTINGS, &lv_font_montserrat_16, COL_DIM);
  lv_obj_center(gl);
  lv_obj_add_event_cb(gear, gear_event, LV_EVENT_LONG_PRESSED, nullptr);

  go(scr);
  s_onHome = true;
  s_sendingTile = nullptr;
  s_msgUntil = 0;
  lv_obj_set_tile_id(s_tv, s_homePage, 0, LV_ANIM_OFF);
  home_update_title();
  s_homeTimer = lv_timer_create(home_timer_cb, 40, nullptr);
}

// ======================================================================
// MENU
// ======================================================================
static void menu_event(lv_event_t *e) {
  Build *b = build_active();
  switch (EV_INT(e)) {
    case 0: show_home(); break;
    case 1: show_builds(); break;
    case 2: show_edit(g_settings.activeBuild); break;
    case 3:
      b->layout = b->layout == LAYOUT_LARGE ? LAYOUT_DENSE : LAYOUT_LARGE;
      storage_save_build(g_settings.activeBuild);
      row_set_text(lv_event_get_target(e), b->layout == LAYOUT_LARGE ? "Layout: Large (4)" : "Layout: Dense (7)");
      break;
    case 4: show_settings(); break;
    case 5: show_customs(); break;
    case 6: show_msgs(); break;
    case 7: show_web(); break;
  }
}

static void back_home(lv_event_t *) { show_home(); }

static lv_obj_t *s_menuNet = nullptr, *s_menuIp = nullptr;
static void net_status_text(char *buf, size_t n);
static void menu_timer_cb(lv_timer_t *) {
  char buf[48];
  net_status_text(buf, sizeof(buf));
  if (strcmp(lv_label_get_text(s_menuIp), buf)) lv_label_set_text(s_menuIp, buf);
}

static void show_menu() {
  Build *b = build_active();
  lv_obj_t *scr = new_screen();
  add_title(scr, "MENU");
  lv_obj_t *l = add_list(scr);
  char buf[48];
  add_row(l, LV_SYMBOL_PLAY "  Back to pad", menu_event, UD(0), COL_YELLOW);
  snprintf(buf, sizeof(buf), LV_SYMBOL_LIST "  Builds (%s)", b->name);
  add_row(l, buf, menu_event, UD(1), COL_YELLOW);
  add_row(l, LV_SYMBOL_EDIT "  Edit stratagems", menu_event, UD(2), COL_YELLOW);
  s_menuNet = add_row(l, LV_SYMBOL_WIFI "  Phone editor", menu_event, UD(7), COL_GREEN, 68);
  lv_obj_t *nm = lv_obj_get_child(s_menuNet, 0);
  lv_obj_align(nm, LV_ALIGN_TOP_LEFT, 0, 5);
  s_menuIp = add_label(s_menuNet, "", &lv_font_montserrat_14, COL_DIM);
  lv_obj_align(s_menuIp, LV_ALIGN_BOTTOM_LEFT, 30, -6);
  add_row(l, b->layout == LAYOUT_LARGE ? "Layout: Large (4)" : "Layout: Dense (7)", menu_event, UD(3), COL_YELLOW);
  add_row(l, LV_SYMBOL_SETTINGS "  Settings", menu_event, UD(4), COL_YELLOW);
  add_row(l, LV_SYMBOL_PLUS "  Custom stratagems", menu_event, UD(5), COL_YELLOW);
  add_row(l, LV_SYMBOL_KEYBOARD "  Chat messages", menu_event, UD(6), COL_YELLOW);
  add_back(scr, back_home);
  go(scr);
  s_screenTimer = lv_timer_create(menu_timer_cb, 500, nullptr);
  menu_timer_cb(nullptr);
}

// ======================================================================
// BUILDS
// ======================================================================
static int s_buildOpts = 0;
static bool s_confirmDelete = false;

static void builds_event(lv_event_t *e) {
  int i = EV_INT(e);
  if (i < 0) {  // new
    char name[NAME_LEN + 1];
    snprintf(name, sizeof(name), "Build %d", g_buildCount + 1);
    int bi = build_add(name);
    if (bi < 0) return;
    storage_save_all_builds();
    show_edit(bi);
    return;
  }
  show_build_opts(i);
}

static void back_menu(lv_event_t *) { show_menu(); }

static void show_builds() {
  lv_obj_t *scr = new_screen();
  char t[24];
  snprintf(t, sizeof(t), "BUILDS %d/%d", g_buildCount, MAX_BUILDS);
  add_title(scr, t);
  lv_obj_t *l = add_list(scr);
  if (g_buildCount < MAX_BUILDS) add_row(l, LV_SYMBOL_PLUS "  New build", builds_event, UD(-1), COL_GREEN);
  for (int i = 0; i < g_buildCount; i++) {
    char buf[48];
    bool act = i == g_settings.activeBuild;
    snprintf(buf, sizeof(buf), "%s%s (%d)", act ? LV_SYMBOL_OK " " : "", g_builds[i].name, g_builds[i].count);
    lv_obj_t *r = add_row(l, buf, builds_event, UD(i), act ? COL_YELLOW : COL_DIM);
    if (act) lv_obj_add_state(r, LV_STATE_CHECKED);
  }
  add_back(scr, back_menu);
  go(scr);
}

static void rename_build_done(const char *txt) {
  if (txt[0]) {
    strncpy(g_builds[s_buildOpts].name, txt, NAME_LEN);
    g_builds[s_buildOpts].name[NAME_LEN] = 0;
    storage_save_build(s_buildOpts);
  }
  show_build_opts(s_buildOpts);
}
static void rename_build_cancel() { show_build_opts(s_buildOpts); }

static void build_opts_event(lv_event_t *e) {
  int bi = s_buildOpts;
  Build &b = g_builds[bi];
  switch (EV_INT(e)) {
    case 0:
      g_settings.activeBuild = bi;
      storage_save_settings();
      s_homePage = 0;
      show_home();
      break;
    case 1: show_edit(bi); break;
    case 2: show_text_input("Rename build", b.name, rename_build_done, rename_build_cancel); break;
    case 3:
      b.layout = b.layout == LAYOUT_LARGE ? LAYOUT_DENSE : LAYOUT_LARGE;
      storage_save_build(bi);
      row_set_text(lv_event_get_target(e), b.layout == LAYOUT_LARGE ? "Layout: Large (4)" : "Layout: Dense (7)");
      break;
    case 4: {
      int n = build_duplicate(bi);
      if (n >= 0) {
        storage_save_all_builds();
        show_build_opts(n);
      }
      break;
    }
    case 5:
      if (g_buildCount <= 1) {
        row_set_text(lv_event_get_target(e), "Can't delete last build");
      } else if (!s_confirmDelete) {
        s_confirmDelete = true;
        row_set_text(lv_event_get_target(e), "Tap again to DELETE");
        lv_obj_set_style_border_color(lv_event_get_target(e), lv_color_hex(COL_RED), 0);
      } else {
        build_delete(bi);
        storage_save_all_builds();
        storage_save_settings();
        show_builds();
      }
      break;
  }
}

static void back_builds(lv_event_t *) { show_builds(); }

static void show_build_opts(int bi) {
  s_buildOpts = bi;
  s_confirmDelete = false;
  Build &b = g_builds[bi];
  lv_obj_t *scr = new_screen();
  add_title(scr, b.name);
  lv_obj_t *l = add_list(scr);
  add_row(l, bi == g_settings.activeBuild ? LV_SYMBOL_PLAY "  Use (active)" : LV_SYMBOL_PLAY "  Use this build", build_opts_event, UD(0), COL_GREEN);
  add_row(l, LV_SYMBOL_EDIT "  Edit stratagems", build_opts_event, UD(1), COL_YELLOW);
  add_row(l, LV_SYMBOL_KEYBOARD "  Rename", build_opts_event, UD(2), COL_YELLOW);
  add_row(l, b.layout == LAYOUT_LARGE ? "Layout: Large (4)" : "Layout: Dense (7)", build_opts_event, UD(3), COL_YELLOW);
  if (g_buildCount < MAX_BUILDS) add_row(l, LV_SYMBOL_COPY "  Duplicate", build_opts_event, UD(4), COL_YELLOW);
  add_row(l, LV_SYMBOL_TRASH "  Delete", build_opts_event, UD(5), COL_RED);
  add_back(scr, back_builds);
  go(scr);
}

// ======================================================================
// EDIT BUILD (reorder / remove / add)
// ======================================================================
static int s_eb = 0;       // build being edited
static int s_sel = -1;     // selected slot
static lv_obj_t *s_editList = nullptr, *s_editTitle = nullptr;

static void edit_refresh() {
  Build &b = g_builds[s_eb];
  lv_label_set_text_fmt(s_editTitle, "%s %d/%d", b.name, b.count, MAX_SLOTS);
  // child 0 is the "add" row; slot k is child k+1
  while ((int)lv_obj_get_child_cnt(s_editList) > b.count + 1)
    lv_obj_del(lv_obj_get_child(s_editList, lv_obj_get_child_cnt(s_editList) - 1));
  for (int k = 0; k < b.count; k++) {
    lv_obj_t *r = lv_obj_get_child(s_editList, k + 1);
    const Strat *s = strat_get(b.ids[k]);
    char buf[48];
    snprintf(buf, sizeof(buf), "%d. %s", k + 1, s ? s->shortName : "(unknown)");
    row_set_text(r, buf);
    lv_obj_set_style_border_color(r, lv_color_hex(s ? cat_color(s->cat) : COL_DIM), 0);
    if (k == s_sel) lv_obj_add_state(r, LV_STATE_CHECKED);
    else lv_obj_clear_state(r, LV_STATE_CHECKED);
  }
}

static void edit_row_event(lv_event_t *e) {
  lv_obj_t *r = lv_event_get_target(e);
  int k = lv_obj_get_index(r) - 1;
  s_sel = (s_sel == k) ? -1 : k;
  edit_refresh();
}

static void edit_add_event(lv_event_t *) { show_categories(s_eb); }

static void edit_action(lv_event_t *e) {
  Build &b = g_builds[s_eb];
  if (s_sel < 0 || s_sel >= b.count) return;
  int a = EV_INT(e);
  if (a == 0 && s_sel > 0) {  // up
    uint16_t t = b.ids[s_sel - 1]; b.ids[s_sel - 1] = b.ids[s_sel]; b.ids[s_sel] = t;
    s_sel--;
  } else if (a == 1 && s_sel < b.count - 1) {  // down
    uint16_t t = b.ids[s_sel + 1]; b.ids[s_sel + 1] = b.ids[s_sel]; b.ids[s_sel] = t;
    s_sel++;
  } else if (a == 2) {  // remove
    for (int k = s_sel; k < b.count - 1; k++) b.ids[k] = b.ids[k + 1];
    b.count--;
    if (s_sel >= b.count) s_sel = b.count - 1;
  } else {
    return;
  }
  storage_save_build(s_eb);
  edit_refresh();
  if (s_sel >= 0) lv_obj_scroll_to_view(lv_obj_get_child(s_editList, s_sel + 1), LV_ANIM_ON);
}

static void edit_back(lv_event_t *) {
  if (s_eb == g_settings.activeBuild) show_menu();
  else show_build_opts(s_eb);
}

static void show_edit(int bi) {
  if (bi != s_eb) s_sel = -1;
  s_eb = bi;
  Build &b = g_builds[bi];
  if (s_sel >= b.count) s_sel = -1;
  lv_obj_t *scr = new_screen();
  s_editTitle = add_title(scr, "");
  // list leaves room for the action bar above the back button
  s_editList = add_list(scr, 58, 232, 296);
  add_row(s_editList, LV_SYMBOL_PLUS "  Add / remove", edit_add_event, nullptr, COL_GREEN);
  for (int k = 0; k < b.count; k++) add_row(s_editList, "", edit_row_event, nullptr);

  // action bar: up / down / remove the selected slot (16 px apart)
  static const char *icons[3] = {LV_SYMBOL_UP, LV_SYMBOL_DOWN, LV_SYMBOL_TRASH};
  for (int a = 0; a < 3; a++) {
    lv_obj_t *btn = add_button(scr, icons[a], 74, 44, edit_action, UD(a));
    lv_obj_align(btn, LV_ALIGN_TOP_MID, (a - 1) * 90, 298);
  }
  add_back(scr, edit_back);
  edit_refresh();
  go(scr);
  if (s_sel >= 0) {
    lv_obj_update_layout(s_editList);
    lv_obj_scroll_to_view(lv_obj_get_child(s_editList, s_sel + 1), LV_ANIM_OFF);
  }
}

// ======================================================================
// CATEGORY + STRATAGEM PICKER
// ======================================================================
static int s_pickBuild = 0, s_pickCat = 0;
static lv_obj_t *s_pickTitle = nullptr;

static int customs_in_cat(int cat) {
  int n = 0;
  for (int j = 0; j < g_customCount; j++) n += custom_find(g_customs[j].id)->cat == cat;
  return n;
}

static int count_in_cat(const Build &b, int cat) {
  int n = 0;
  for (int k = 0; k < b.count; k++) {
    const Strat *s = strat_get(b.ids[k]);
    if (s && s->cat == cat) n++;
  }
  return n;
}

static void cat_event(lv_event_t *e) { show_picker(s_pickBuild, EV_INT(e)); }
static void cat_back(lv_event_t *) { show_edit(s_pickBuild); }

static void show_categories(int bi) {
  s_pickBuild = bi;
  Build &b = g_builds[bi];
  lv_obj_t *scr = new_screen();
  char t[32];
  snprintf(t, sizeof(t), "ADD  %d/%d", b.count, MAX_SLOTS);
  add_title(scr, t);
  lv_obj_t *l = add_list(scr);
  for (int c = 0; c < CAT_COUNT; c++) {
    if (c == CAT_CUSTOM && !customs_in_cat(c)) continue;  // custom ones may live in the regular categories
    if (c == CAT_MESSAGE && g_msgCount == 0) continue;
    char buf[40];
    int n = count_in_cat(b, c);
    if (n) snprintf(buf, sizeof(buf), "%s  (%d)", cat_name(c), n);
    else snprintf(buf, sizeof(buf), "%s", cat_name(c));
    add_row(l, buf, cat_event, UD(c), cat_color(c));
  }
  add_back(scr, cat_back);
  go(scr);
}

static void picker_row_text(lv_obj_t *row, const Strat *s, bool on) {
  char buf[64];
  snprintf(buf, sizeof(buf), "%s%s", on ? LV_SYMBOL_OK " " : "", s->name);
  lv_label_set_text(lv_obj_get_child(row, 0), buf);
  if (on) lv_obj_add_state(row, LV_STATE_CHECKED);
  else lv_obj_clear_state(row, LV_STATE_CHECKED);
}

static void picker_title() {
  Build &b = g_builds[s_pickBuild];
  lv_label_set_text_fmt(s_pickTitle, "%s %d/%d", cat_name(s_pickCat), b.count, MAX_SLOTS);
}

static void picker_event(lv_event_t *e) {
  lv_obj_t *row = lv_event_get_target(e);
  uint16_t id = (uint16_t)EV_INT(e);
  Build &b = g_builds[s_pickBuild];
  bool was = build_find_slot(&b, id) >= 0;
  if (!was && b.count >= MAX_SLOTS) {
    lv_label_set_text(s_pickTitle, "#E5533D Build full#");
    return;
  }
  bool on = build_toggle(&b, id);
  storage_save_build(s_pickBuild);
  picker_row_text(row, strat_get(id), on);
  picker_title();
}

static void picker_back(lv_event_t *) { show_categories(s_pickBuild); }

static lv_obj_t *picker_add(lv_obj_t *l, const Strat *s, const Build &b) {
  lv_obj_t *r = add_row(l, "", picker_event, UD(s->id), cat_color(s->cat), 62);
  lv_obj_t *name = lv_obj_get_child(r, 0);
  lv_obj_set_style_text_font(name, &lv_font_montserrat_16, 0);
  lv_obj_align(name, LV_ALIGN_TOP_LEFT, 0, 9);
  char arrows[4 * MAX_CODE + 8];
  if (s->cat == CAT_MESSAGE) strcpy(arrows, LV_SYMBOL_KEYBOARD "  chat message");
  else strat_code_to_arrows(s->code, arrows, sizeof(arrows));
  lv_obj_t *a = add_label(r, arrows, &lv_font_montserrat_14, cat_color(s->cat));
  lv_obj_align(a, LV_ALIGN_BOTTOM_LEFT, 0, -8);
  picker_row_text(r, s, build_find_slot(&b, s->id) >= 0);
  return r;
}

static void show_picker(int bi, int cat) {
  s_pickBuild = bi;
  s_pickCat = cat;
  Build &b = g_builds[bi];
  lv_obj_t *scr = new_screen();
  s_pickTitle = add_title(scr, "");
  lv_label_set_recolor(s_pickTitle, true);
  picker_title();
  lv_obj_t *l = add_list(scr);
  if (cat == CAT_MESSAGE) {
    for (int j = 0; j < g_msgCount; j++) picker_add(l, msg_find(g_msgs[j].id), b);
  } else {
    for (size_t i = 0; i < strat_catalog_count(); i++) {
      const Strat *s = strat_catalog_at(i);
      if (s->cat == cat) picker_add(l, s, b);
    }
    for (int j = 0; j < g_customCount; j++) {  // made on the pad, or imported into this category
      const Strat *s = custom_find(g_customs[j].id);
      if (s->cat == cat) picker_add(l, s, b);
    }
  }
  add_back(scr, picker_back);
  go(scr);
}

// ======================================================================
// SETTINGS
// ======================================================================
enum { SET_WIFI, SET_HOMEWIFI, SET_OUTPUT, SET_DONGLE, SET_MENUKEY, SET_MODE, SET_DIRS, SET_SPEED, SET_CHATKEY,
       SET_TEXTLAYOUT, SET_BRIGHT, SET_ROTATE, SET_CODES, SET_SERIAL, SET_TEST, SET_RESTART, SET_RESET, SET_N };
static bool s_confirmForget = false;
static bool s_confirmReset = false;
static lv_obj_t *s_dongleRow = nullptr;

static void dongle_text(char *buf, size_t n) {
  if (!link_enabled()) snprintf(buf, n, "Dongle: Output is USB");
  else if (link_pair_seconds_left()) snprintf(buf, n, "Pairing... %ds (tap: stop)", link_pair_seconds_left());
  else if (link_connected()) snprintf(buf, n, "Dongle: connected");
  else if (link_dongle_seen()) snprintf(buf, n, "Dongle: no USB host");
  else if (link_paired()) snprintf(buf, n, "Dongle: searching (tap: pair)");
  else snprintf(buf, n, "Dongle: tap to pair");
}

static void settings_timer_cb(lv_timer_t *) {  // dongle state changes on its own
  char buf[48];
  dongle_text(buf, sizeof(buf));
  lv_obj_t *lbl = lv_obj_get_child(s_dongleRow, 0);
  if (strcmp(lv_label_get_text(lbl), buf)) lv_label_set_text(lbl, buf);
}

static void setting_text(int id, char *buf, size_t n) {
  switch (id) {
    case SET_OUTPUT: snprintf(buf, n, "Output: %s", g_settings.output == 1 ? "Wireless dongle" : "USB cable"); break;
    case SET_DONGLE: dongle_text(buf, n); break;
    case SET_WIFI: snprintf(buf, n, LV_SYMBOL_WIFI "  Wi-Fi: %s", g_settings.wifiOn ? "On" : "Off"); break;
    case SET_HOMEWIFI:
      if (s_confirmForget) snprintf(buf, n, "Tap again to forget");
      else if (web_home_ssid()[0]) snprintf(buf, n, "Network: %s", web_home_ssid());
      else snprintf(buf, n, "Network: not set");
      break;
    case SET_MENUKEY: snprintf(buf, n, "Menu key: %s", MENU_KEYS[g_settings.menuKey % MENU_KEY_COUNT].label); break;
    case SET_MODE: snprintf(buf, n, "Menu mode: %s", g_settings.menuMode ? "Tap" : "Hold"); break;
    case SET_DIRS: snprintf(buf, n, "Arrows: %s", DIR_SETS[g_settings.dirSet % DIR_SET_COUNT].label); break;
    case SET_SPEED: snprintf(buf, n, "Speed: %s", SPEEDS[g_settings.speed % SPEED_COUNT].label); break;
    case SET_CHATKEY: snprintf(buf, n, "Chat key: %s", CHAT_KEYS[g_settings.chatKey % CHAT_KEY_COUNT].label); break;
    case SET_TEXTLAYOUT: snprintf(buf, n, "Text layout: %s", MACRO_LAYOUTS[g_settings.textLayout % MACRO_LAYOUT_COUNT].label); break;
    case SET_BRIGHT: snprintf(buf, n, "Brightness: %d%%", g_settings.brightness); break;
    case SET_ROTATE: snprintf(buf, n, "Rotate screen: %s", g_settings.rotate180 ? "180" : "Off"); break;
    case SET_CODES: snprintf(buf, n, "Codes on tiles: %s", g_settings.showCodes ? "On" : "Off"); break;
    case SET_SERIAL: snprintf(buf, n, "USB serial: %s", g_settings.usbSerial ? "On" : "Off"); break;
    case SET_TEST: snprintf(buf, n, LV_SYMBOL_UPLOAD "  Test (Reinforce)"); break;
    case SET_RESTART: snprintf(buf, n, LV_SYMBOL_REFRESH "  Restart"); break;
    case SET_RESET: snprintf(buf, n, s_confirmReset ? "Tap again to WIPE" : LV_SYMBOL_WARNING "  Factory reset"); break;
  }
}

static void settings_event(lv_event_t *e) {
  int id = EV_INT(e);
  lv_obj_t *row = lv_event_get_target(e);
  switch (id) {
    case SET_WIFI: web_set_enabled(!g_settings.wifiOn); break;
    case SET_HOMEWIFI:  // forget the saved home network (set a new one from the phone editor)
      if (!web_home_ssid()[0]) return;
      if (!s_confirmForget) {
        s_confirmForget = true;
        lv_obj_set_style_border_color(row, lv_color_hex(COL_RED), 0);
      } else {
        s_confirmForget = false;
        web_forget_home();
        lv_obj_set_style_border_color(row, lv_color_hex(COL_YELLOW), 0);
      }
      break;
    case SET_OUTPUT:
      link_set_enabled(g_settings.output != 1);
      settings_timer_cb(nullptr);
      break;
    case SET_DONGLE:
      if (!link_enabled()) return;
      if (link_pair_seconds_left()) link_pair_cancel();
      else link_pair_start();
      settings_timer_cb(nullptr);
      return;
    case SET_MENUKEY: g_settings.menuKey = (g_settings.menuKey + 1) % MENU_KEY_COUNT; break;
    case SET_MODE: g_settings.menuMode ^= 1; break;
    case SET_DIRS: g_settings.dirSet = (g_settings.dirSet + 1) % DIR_SET_COUNT; break;
    case SET_SPEED: g_settings.speed = (g_settings.speed + 1) % SPEED_COUNT; break;
    case SET_BRIGHT:
      g_settings.brightness = g_settings.brightness >= 100 ? 20 : g_settings.brightness + 20;
      display_set_brightness(g_settings.brightness);
      break;
    case SET_CHATKEY: g_settings.chatKey = (g_settings.chatKey + 1) % CHAT_KEY_COUNT; break;
    case SET_TEXTLAYOUT: g_settings.textLayout = (g_settings.textLayout + 1) % MACRO_LAYOUT_COUNT; break;
    case SET_ROTATE:
      g_settings.rotate180 ^= 1;
      display_set_rotation(g_settings.rotate180);
      break;
    case SET_CODES: g_settings.showCodes ^= 1; break;
    case SET_SERIAL: g_settings.usbSerial ^= 1; break;
    case SET_TEST: {
      const Strat *s = strat_get(200);
      char buf[32];
      snprintf(buf, sizeof(buf), "%s not connected", hid_output_name());
      if (!hid_connected()) row_set_text(row, buf);
      else if (s && hid_send(s->code)) row_set_text(row, "Sent: " LV_SYMBOL_UP LV_SYMBOL_DOWN LV_SYMBOL_RIGHT LV_SYMBOL_LEFT LV_SYMBOL_UP);
      return;
    }
    case SET_RESTART: ESP.restart(); return;
    case SET_RESET:
      if (!s_confirmReset) {
        s_confirmReset = true;
        lv_obj_set_style_border_color(row, lv_color_hex(COL_RED), 0);
        break;
      }
      storage_factory_reset();
      ESP.restart();
      return;
  }
  storage_save_settings();
  char buf[48];
  setting_text(id, buf, sizeof(buf));
  if (id == SET_SERIAL) strncat(buf, " (restart)", sizeof(buf) - strlen(buf) - 1);
  row_set_text(row, buf);
}

static void show_settings() {
  s_confirmReset = false;
  s_confirmForget = false;
  lv_obj_t *scr = new_screen();
  add_title(scr, "SETTINGS");
  lv_obj_t *l = add_list(scr);
  for (int id = 0; id < SET_N; id++) {
    char buf[48];
    setting_text(id, buf, sizeof(buf));
    uint32_t accent = id >= SET_TEST ? (id == SET_RESET ? COL_RED : COL_GREEN) : (id <= SET_DONGLE ? COL_GREEN : COL_YELLOW);
    lv_obj_t *r = add_row(l, buf, settings_event, UD(id), accent);
    lv_obj_set_style_text_font(lv_obj_get_child(r, 0), &lv_font_montserrat_16, 0);
    if (id == SET_DONGLE) s_dongleRow = r;
  }
  add_back(scr, back_menu);
  go(scr);
  s_screenTimer = lv_timer_create(settings_timer_cb, 500, nullptr);
}

// ======================================================================
// CUSTOM STRATAGEMS
// ======================================================================
static int s_customSel = -1;
static bool s_confirmCustomDel = false;
static char s_pendingName[NAME_LEN + 1];

static void new_custom_name_done(const char *txt) {
  strncpy(s_pendingName, txt[0] ? txt : "Custom", NAME_LEN);
  s_pendingName[NAME_LEN] = 0;
  show_code_editor(-1);
}
static void new_custom_cancel() { show_customs(); }

static void customs_event(lv_event_t *e) {
  int j = EV_INT(e);
  if (j < 0) show_text_input("Custom name", "", new_custom_name_done, new_custom_cancel);
  else show_custom_opts(j);
}

static void show_customs() {
  lv_obj_t *scr = new_screen();
  char t[24];
  snprintf(t, sizeof(t), "CUSTOM %d/%d", g_customCount, MAX_CUSTOM);
  add_title(scr, t);
  lv_obj_t *l = add_list(scr);
  if (g_customCount < MAX_CUSTOM) add_row(l, LV_SYMBOL_PLUS "  New stratagem", customs_event, UD(-1), COL_GREEN);
  for (int j = 0; j < g_customCount; j++) {
    uint32_t col = cat_color(custom_find(g_customs[j].id)->cat);
    lv_obj_t *r = add_row(l, g_customs[j].name, customs_event, UD(j), col, 62);
    lv_obj_t *name = lv_obj_get_child(r, 0);
    lv_obj_set_style_text_font(name, &lv_font_montserrat_16, 0);
    lv_obj_align(name, LV_ALIGN_TOP_LEFT, 0, 9);
    char arrows[4 * MAX_CODE + 8];
    strat_code_to_arrows(g_customs[j].code, arrows, sizeof(arrows));
    lv_obj_t *a = add_label(r, arrows[0] ? arrows : "no code yet", &lv_font_montserrat_14, arrows[0] ? col : COL_DIM);
    lv_obj_align(a, LV_ALIGN_BOTTOM_LEFT, 0, -8);
  }
  add_back(scr, back_menu);
  go(scr);
}

static void rename_custom_done(const char *txt) {
  if (txt[0]) {
    strncpy(g_customs[s_customSel].name, txt, NAME_LEN);
    g_customs[s_customSel].name[NAME_LEN] = 0;
    custom_refresh(s_customSel);
    storage_save_customs();
  }
  show_custom_opts(s_customSel);
}
static void rename_custom_cancel() { show_custom_opts(s_customSel); }

static void custom_opts_event(lv_event_t *e) {
  int j = s_customSel;
  switch (EV_INT(e)) {
    case 0: show_code_editor(j); break;
    case 1: show_text_input("Rename", g_customs[j].name, rename_custom_done, rename_custom_cancel); break;
    case 2:
      if (!s_confirmCustomDel) {
        s_confirmCustomDel = true;
        row_set_text(lv_event_get_target(e), "Tap again to DELETE");
      } else {
        custom_delete(j);
        storage_save_customs();
        show_customs();
      }
      break;
  }
}

static void back_customs(lv_event_t *) { show_customs(); }

static void show_custom_opts(int j) {
  s_customSel = j;
  s_confirmCustomDel = false;
  lv_obj_t *scr = new_screen();
  add_title(scr, g_customs[j].name);
  lv_obj_t *l = add_list(scr);
  add_row(l, LV_SYMBOL_EDIT "  Edit code", custom_opts_event, UD(0), COL_YELLOW);
  add_row(l, LV_SYMBOL_KEYBOARD "  Rename", custom_opts_event, UD(1), COL_YELLOW);
  add_row(l, LV_SYMBOL_TRASH "  Delete", custom_opts_event, UD(2), COL_RED);
  add_back(scr, back_customs);
  go(scr);
}

// ---- arrow code editor (D-pad) ----
static char s_code[MAX_CODE + 1];
static int s_codeJ = -1;
static lv_obj_t *s_codeLabel = nullptr;

static void code_refresh() {
  if (!s_code[0]) {
    lv_label_set_text(s_codeLabel, "enter code");
    lv_obj_set_style_text_color(s_codeLabel, lv_color_hex(COL_DIM), 0);
    return;
  }
  char arrows[4 * MAX_CODE + 8];
  strat_code_to_arrows(s_code, arrows, sizeof(arrows));
  lv_label_set_text(s_codeLabel, arrows);
  lv_obj_set_style_text_color(s_codeLabel, lv_color_hex(cat_color(CAT_CUSTOM)), 0);
}

static void code_dir(lv_event_t *e) {
  size_t n = strlen(s_code);
  if (n >= MAX_CODE) return;
  s_code[n] = (char)EV_INT(e);
  s_code[n + 1] = 0;
  code_refresh();
}

static void code_del(lv_event_t *) {
  size_t n = strlen(s_code);
  if (n) s_code[n - 1] = 0;
  code_refresh();
}

static void code_save(lv_event_t *) {
  if (!s_code[0]) return;
  if (s_codeJ < 0) {
    int j = custom_add(s_pendingName, s_code);
    if (j >= 0) storage_save_customs();
    show_customs();
  } else {
    strncpy(g_customs[s_codeJ].code, s_code, MAX_CODE);
    g_customs[s_codeJ].code[MAX_CODE] = 0;
    custom_refresh(s_codeJ);
    storage_save_customs();
    show_custom_opts(s_codeJ);
  }
}

static void code_cancel(lv_event_t *) {
  if (s_codeJ < 0) show_customs();
  else show_custom_opts(s_codeJ);
}

static void show_code_editor(int j) {
  s_codeJ = j;
  strncpy(s_code, j >= 0 ? g_customs[j].code : "", MAX_CODE);
  s_code[MAX_CODE] = 0;
  lv_obj_t *scr = new_screen();
  lv_obj_t *nm = add_label(scr, j >= 0 ? g_customs[j].name : s_pendingName, &lv_font_montserrat_16, COL_YELLOW);
  lv_obj_align(nm, LV_ALIGN_TOP_MID, 0, 18);
  s_codeLabel = add_label(scr, "", &lv_font_montserrat_24, COL_DIM);
  lv_obj_set_style_text_letter_space(s_codeLabel, 2, 0);
  lv_obj_align(s_codeLabel, LV_ALIGN_TOP_MID, 0, 50);

  // D-pad centred at (0, +24), 74 px buttons, >= 18 px apart
  struct { char c; const char *sym; int x, y; } pad[4] = {
    {'U', LV_SYMBOL_UP, 0, -68}, {'D', LV_SYMBOL_DOWN, 0, 116}, {'L', LV_SYMBOL_LEFT, -92, 24}, {'R', LV_SYMBOL_RIGHT, 92, 24}};
  for (auto &p : pad) {
    lv_obj_t *b = add_button(scr, p.sym, 74, 74, code_dir, UD(p.c));
    lv_obj_set_style_radius(b, 16, 0);
    lv_obj_set_style_text_font(b, &lv_font_montserrat_28, 0);
    lv_obj_align(b, LV_ALIGN_CENTER, p.x, p.y);
  }
  lv_obj_t *del = add_button(scr, LV_SYMBOL_BACKSPACE, 54, 54, code_del, nullptr);
  lv_obj_set_style_radius(del, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_color(del, lv_color_hex(COL_DIM), 0);
  lv_obj_set_style_text_color(del, lv_color_hex(COL_DIM), 0);
  lv_obj_align(del, LV_ALIGN_CENTER, 0, 24);

  lv_obj_t *cancel = add_button(scr, LV_SYMBOL_CLOSE, 84, 46, code_cancel, nullptr);
  lv_obj_set_style_border_color(cancel, lv_color_hex(COL_RED), 0);
  lv_obj_set_style_text_color(cancel, lv_color_hex(COL_RED), 0);
  lv_obj_align(cancel, LV_ALIGN_CENTER, -116, 128);
  lv_obj_t *save = add_button(scr, LV_SYMBOL_OK " Save", 96, 46, code_save, nullptr);
  lv_obj_set_style_border_color(save, lv_color_hex(COL_GREEN), 0);
  lv_obj_set_style_text_color(save, lv_color_hex(COL_GREEN), 0);
  lv_obj_align(save, LV_ALIGN_CENTER, 112, 128);

  code_refresh();
  go(scr);
}

// ======================================================================
// PHONE EDITOR (Wi-Fi status + QR codes)
// ======================================================================
static lv_obj_t *s_qrBox = nullptr, *s_qr = nullptr, *s_webInfo = nullptr;
static char s_webShown[96];  // what the QR currently encodes, to avoid redrawing it
static uint32_t s_webSaves = 0, s_webBannerUntil = 0;

static void web_show_qr(const char *data) {
  lv_obj_clear_flag(s_qrBox, LV_OBJ_FLAG_HIDDEN);
  if (strcmp(data, s_webShown)) {
    strncpy(s_webShown, data, sizeof(s_webShown) - 1);
    lv_qrcode_update(s_qr, data, strlen(data));
  }
}

static void web_timer_cb(lv_timer_t *) {
  if (web_save_count() != s_webSaves) {
    s_webSaves = web_save_count();
    s_webBannerUntil = millis() + 4000;
    lv_label_set_text(s_webInfo, "#66C15A " LV_SYMBOL_OK " Saved from phone#\n#8A929C Keep editing, or tap Done#");
  }
  if (millis() < s_webBannerUntil) return;

  char buf[96];
  switch (web_state()) {
    case NET_ONLINE:
      web_show_qr(web_url());
      lv_label_set_text_fmt(s_webInfo, "Scan to open the editor\n#8A929C %s  " LV_SYMBOL_WIFI " %s#", web_ip(), web_home_ssid());
      break;
    case NET_CONNECTING:
      lv_obj_add_flag(s_qrBox, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text_fmt(s_webInfo, "Connecting to\n#FFE710 %s#...", web_home_ssid());
      break;
    case NET_SETUP:
      if (web_ap_clients() == 0) {
        snprintf(buf, sizeof(buf), "WIFI:T:WPA;S:%s;P:%s;;", web_ap_ssid(), web_ap_pass());
        web_show_qr(buf);
        lv_label_set_text_fmt(s_webInfo, "1. Scan to join the pad\n#8A929C %s  " LV_SYMBOL_RIGHT "  %s#", web_ap_ssid(), web_ap_pass());
      } else {
        web_show_qr(web_url());
        lv_label_set_text_fmt(s_webInfo, "2. Scan, then pick your Wi-Fi\n#8A929C %s#", web_url());
      }
      break;
    default:
      lv_obj_add_flag(s_qrBox, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text(s_webInfo, "Wi-Fi is off");
      break;
  }
}

static void web_done(lv_event_t *) { show_home(); }

static void show_web() {
  if (!web_enabled()) web_set_enabled(true);  // opening the editor turns Wi-Fi on (and keeps it on)
  lv_obj_t *scr = new_screen();
  add_title(scr, "PHONE EDITOR");

  // QR needs a light quiet zone around it to scan on a dark screen
  s_qrBox = lv_obj_create(scr);
  lv_obj_remove_style_all(s_qrBox);
  lv_obj_set_size(s_qrBox, 200, 200);
  lv_obj_set_style_bg_color(s_qrBox, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(s_qrBox, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(s_qrBox, 10, 0);
  lv_obj_align(s_qrBox, LV_ALIGN_CENTER, 0, -12);
  lv_obj_clear_flag(s_qrBox, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(s_qrBox, LV_OBJ_FLAG_HIDDEN);
  s_qr = lv_qrcode_create(s_qrBox, 176, lv_color_black(), lv_color_white());
  lv_obj_center(s_qr);
  s_webShown[0] = 0;

  s_webInfo = lv_label_create(scr);
  lv_label_set_recolor(s_webInfo, true);
  lv_obj_set_width(s_webInfo, 280);
  lv_obj_set_style_text_align(s_webInfo, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(s_webInfo, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(s_webInfo, lv_color_white(), 0);
  lv_obj_align(s_webInfo, LV_ALIGN_TOP_MID, 0, 300);

  lv_obj_t *done = add_button(scr, LV_SYMBOL_OK "  Done", 130, 46, web_done, nullptr);
  lv_obj_align(done, LV_ALIGN_BOTTOM_MID, 0, -14);

  go(scr);
  s_onWeb = true;
  s_webSaves = web_save_count();
  s_webBannerUntil = 0;
  s_screenTimer = lv_timer_create(web_timer_cb, 250, nullptr);
  web_timer_cb(nullptr);
}

// Short Wi-Fi status for the menu row
static void net_status_text(char *buf, size_t n) {
  switch (web_state()) {
    case NET_ONLINE: snprintf(buf, n, "%s", web_ip()); break;
    case NET_CONNECTING: snprintf(buf, n, "connecting to %s", web_home_ssid()); break;
    case NET_SETUP: snprintf(buf, n, "setup: join %s", web_ap_ssid()); break;
    default: snprintf(buf, n, "Wi-Fi off"); break;
  }
}

// ======================================================================
// CHAT MESSAGES
// ======================================================================
static int s_msgSel = -1;
static bool s_confirmMsgDel = false;

static void new_msg_done(const char *txt) {
  if (txt[0] && msg_add(txt) >= 0) storage_save_msgs();
  show_msgs();
}
static void msgs_cancel() { show_msgs(); }

static void msgs_event(lv_event_t *e) {
  int j = EV_INT(e);
  if (j < 0) show_text_input("New message", "", new_msg_done, msgs_cancel, MSG_LEN);
  else show_msg_opts(j);
}

static void show_msgs() {
  lv_obj_t *scr = new_screen();
  char t[24];
  snprintf(t, sizeof(t), "MESSAGES %d/%d", g_msgCount, MAX_MSG);
  add_title(scr, t);
  lv_obj_t *l = add_list(scr);
  if (g_msgCount < MAX_MSG) add_row(l, LV_SYMBOL_PLUS "  New message", msgs_event, UD(-1), COL_GREEN);
  for (int j = 0; j < g_msgCount; j++) {
    lv_obj_t *r = add_row(l, g_msgs[j].text, msgs_event, UD(j), cat_color(CAT_MESSAGE));
    lv_obj_set_style_text_font(lv_obj_get_child(r, 0), &lv_font_montserrat_16, 0);
  }
  add_back(scr, back_menu);
  go(scr);
}

static void edit_msg_done(const char *txt) {
  if (txt[0]) {
    strncpy(g_msgs[s_msgSel].text, txt, MSG_LEN);
    g_msgs[s_msgSel].text[MSG_LEN] = 0;
    msg_refresh(s_msgSel);
    storage_save_msgs();
  }
  show_msg_opts(s_msgSel);
}
static void edit_msg_cancel() { show_msg_opts(s_msgSel); }

static void msg_opts_event(lv_event_t *e) {
  int j = s_msgSel;
  switch (EV_INT(e)) {
    case 0: show_text_input("Edit message", g_msgs[j].text, edit_msg_done, edit_msg_cancel, MSG_LEN); break;
    case 1:
      if (!s_confirmMsgDel) {
        s_confirmMsgDel = true;
        row_set_text(lv_event_get_target(e), "Tap again to DELETE");
      } else {
        msg_delete(j);
        storage_save_msgs();
        show_msgs();
      }
      break;
  }
}

static void back_msgs(lv_event_t *) { show_msgs(); }

static void show_msg_opts(int j) {
  s_msgSel = j;
  s_confirmMsgDel = false;
  lv_obj_t *scr = new_screen();
  add_title(scr, g_msgs[j].text);
  lv_obj_t *l = add_list(scr);
  add_row(l, LV_SYMBOL_EDIT "  Edit text", msg_opts_event, UD(0), COL_YELLOW);
  add_row(l, LV_SYMBOL_TRASH "  Delete", msg_opts_event, UD(1), COL_RED);
  add_back(scr, back_msgs);
  go(scr);
}

// ======================================================================
// TEXT INPUT (keyboard)
// ======================================================================
static TextDoneCb s_textOk = nullptr;
static void (*s_textCancel)() = nullptr;
static lv_obj_t *s_ta = nullptr;

static void text_ok(lv_event_t *) {
  char buf[MSG_LEN + 1];
  strncpy(buf, lv_textarea_get_text(s_ta), MSG_LEN);
  buf[MSG_LEN] = 0;
  // trim trailing spaces
  for (int n = strlen(buf); n > 0 && buf[n - 1] == ' '; n--) buf[n - 1] = 0;
  if (s_textOk) s_textOk(buf);
}

static void text_cancel(lv_event_t *) {
  if (s_textCancel) s_textCancel();
}

static void kb_event(lv_event_t *e) {
  lv_event_code_t c = lv_event_get_code(e);
  if (c == LV_EVENT_READY) text_ok(e);
  else if (c == LV_EVENT_CANCEL) text_cancel(e);
}

static void show_text_input(const char *title, const char *initial, TextDoneCb ok, void (*cancel)(), int maxLen) {
  s_textOk = ok;
  s_textCancel = cancel;
  lv_obj_t *scr = new_screen();
  lv_obj_t *t = add_label(scr, title, &lv_font_montserrat_16, COL_YELLOW);
  lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 26);

  s_ta = lv_textarea_create(scr);
  lv_textarea_set_one_line(s_ta, true);
  lv_textarea_set_max_length(s_ta, maxLen);
  lv_textarea_set_text(s_ta, initial);
  lv_obj_set_width(s_ta, 280);
  lv_obj_set_style_text_font(s_ta, &lv_font_montserrat_20, 0);
  lv_obj_align(s_ta, LV_ALIGN_TOP_MID, 0, 56);
  lv_obj_add_state(s_ta, LV_STATE_FOCUSED);

  lv_obj_t *kb = lv_keyboard_create(scr);
  lv_obj_set_size(kb, 336, 180);
  lv_obj_align(kb, LV_ALIGN_TOP_MID, 0, 120);
  lv_obj_set_style_text_font(kb, &lv_font_montserrat_16, 0);
  lv_obj_set_style_pad_all(kb, 4, 0);
  lv_obj_set_style_pad_gap(kb, 4, 0);
  lv_keyboard_set_textarea(kb, s_ta);
  lv_obj_add_event_cb(kb, kb_event, LV_EVENT_ALL, nullptr);

  lv_obj_t *cancelB = add_button(scr, LV_SYMBOL_CLOSE, 100, 44, text_cancel, nullptr);
  lv_obj_set_style_border_color(cancelB, lv_color_hex(COL_RED), 0);
  lv_obj_set_style_text_color(cancelB, lv_color_hex(COL_RED), 0);
  lv_obj_align(cancelB, LV_ALIGN_TOP_MID, -62, 314);
  lv_obj_t *okB = add_button(scr, LV_SYMBOL_OK, 100, 44, text_ok, nullptr);
  lv_obj_set_style_border_color(okB, lv_color_hex(COL_GREEN), 0);
  lv_obj_set_style_text_color(okB, lv_color_hex(COL_GREEN), 0);
  lv_obj_align(okB, LV_ALIGN_TOP_MID, 62, 314);
  go(scr);
}

// ======================================================================
void ui_begin() {
  styles_init();
  show_home();
}

void ui_poll() {
  static uint32_t seen = 0;
  if (web_save_count() == seen) return;
  seen = web_save_count();
  if (s_onWeb) return;  // the editor screen shows its own banner
  // Any other screen may hold stale build indices: go back to the pad
  if (g_settings.activeBuild >= g_buildCount) g_settings.activeBuild = 0;
  s_homePage = 0;
  show_home();
  home_flash(LV_SYMBOL_OK " Updated from phone", COL_GREEN, 2500);
}

void ui_boot_button() {
  if (s_onHome) show_menu();
  else show_home();
}
