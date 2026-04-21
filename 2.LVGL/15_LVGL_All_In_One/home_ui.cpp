#include "home_ui.h"

#include "all_in_one_app.h"
#include "img/img_index.h"

namespace {

// Layout constants for the two-page home screen.
constexpr lv_coord_t kScreenWidth = 320;
constexpr lv_coord_t kScreenHeight = 240;
constexpr lv_coord_t kHeaderLogoX = 12;
constexpr lv_coord_t kHeaderLogoY = 4;
constexpr lv_coord_t kHeaderTimeX = 140;
constexpr lv_coord_t kHeaderTimeY = 10;
constexpr lv_coord_t kHeaderWifiX = 286;
constexpr lv_coord_t kHeaderWifiY = 10;

constexpr lv_coord_t kTileOneX[6] = {24, 120, 216, 24, 120, 216};
constexpr lv_coord_t kTileOneY[6] = {45, 45, 45, 147, 147, 147};
constexpr lv_coord_t kTileSize = 80;

constexpr lv_coord_t kPageTwoIconX = 24;
constexpr lv_coord_t kPageTwoIconY = 45;
constexpr lv_coord_t kPageTwoFileX = 120;
constexpr lv_coord_t kPageTwoFileY = 45;

// Page 1 hosts the six most frequently used demo entries.
const AppScreen kPageOneTargets[6] = {
  APP_SCREEN_HEARTRATE,
  APP_SCREEN_CAMERA,
  APP_SCREEN_MUSIC,
  APP_SCREEN_WIFI,
  APP_SCREEN_BUZZER,
  APP_SCREEN_GALLERY,
};

const lv_img_dsc_t *kPageOneIcons[6] = {
  APP_IMG_HEARTRATE,
  APP_IMG_CAM,
  APP_IMG_MUSIC,
  APP_IMG_WIFI,
  APP_IMG_RING,
  APP_IMG_GALLERY,
};

// Shared styles are initialized once and then reused.
static lv_style_t s_screen_style;
static lv_style_t s_tile_style;
static lv_style_t s_icon_style;
static lv_style_t s_icon_pressed_style;
static lv_style_t s_symbol_tile_style;
static lv_style_t s_symbol_tile_pressed_style;
static lv_style_t s_symbol_label_style;
static lv_style_t s_time_style;
static lv_style_t s_wifi_style;
static bool s_styles_ready = false;
static uint8_t s_current_page = 0;

static void create_styles(void);
static void build_header(lv_obj_t *parent, lv_obj_t **time_label, lv_obj_t **wifi_label);
static lv_obj_t *create_icon_button(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, const lv_img_dsc_t *img, AppScreen screen);
static lv_obj_t *create_symbol_button(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, const char *text, AppScreen screen);
static void tileview_event_handler(lv_event_t *e);
static void app_icon_event_handler(lv_event_t *e);

void create_styles(void) {
  if (s_styles_ready) {
    return;
  }

  lv_style_init(&s_screen_style);
  lv_style_set_bg_color(&s_screen_style, lv_color_hex(0xFFFFFF));
  lv_style_set_bg_opa(&s_screen_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_screen_style, 0);
  lv_style_set_pad_all(&s_screen_style, 0);
  lv_style_set_radius(&s_screen_style, 0);
  lv_style_set_shadow_width(&s_screen_style, 0);
  lv_style_set_outline_width(&s_screen_style, 0);

  lv_style_init(&s_tile_style);
  lv_style_set_bg_color(&s_tile_style, lv_color_hex(0xFFFFFF));
  lv_style_set_bg_opa(&s_tile_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_tile_style, 0);
  lv_style_set_pad_all(&s_tile_style, 0);
  lv_style_set_radius(&s_tile_style, 0);
  lv_style_set_shadow_width(&s_tile_style, 0);
  lv_style_set_outline_width(&s_tile_style, 0);

  lv_style_init(&s_icon_style);
  lv_style_set_bg_opa(&s_icon_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_icon_style, 0);
  lv_style_set_shadow_width(&s_icon_style, 0);
  lv_style_set_outline_width(&s_icon_style, 0);
  lv_style_set_pad_all(&s_icon_style, 0);

  lv_style_init(&s_icon_pressed_style);
  lv_style_set_translate_y(&s_icon_pressed_style, 4);
  lv_style_set_img_opa(&s_icon_pressed_style, LV_OPA_80);

  lv_style_init(&s_time_style);
  lv_style_set_text_color(&s_time_style, lv_color_hex(0x111111));
  lv_style_set_text_font(&s_time_style, &lv_font_montserrat_20);

  lv_style_init(&s_wifi_style);
  lv_style_set_text_color(&s_wifi_style, lv_color_hex(0x2793E6));
  lv_style_set_text_font(&s_wifi_style, &lv_font_montserrat_18);

  lv_style_init(&s_symbol_tile_style);
  lv_style_set_bg_color(&s_symbol_tile_style, lv_color_hex(0xE8F1FB));
  lv_style_set_bg_opa(&s_symbol_tile_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_symbol_tile_style, 0);
  lv_style_set_radius(&s_symbol_tile_style, 18);
  lv_style_set_shadow_width(&s_symbol_tile_style, 0);

  lv_style_init(&s_symbol_tile_pressed_style);
  lv_style_set_translate_y(&s_symbol_tile_pressed_style, 4);
  lv_style_set_bg_opa(&s_symbol_tile_pressed_style, LV_OPA_80);

  lv_style_init(&s_symbol_label_style);
  lv_style_set_text_color(&s_symbol_label_style, lv_color_hex(0x1769AA));
  lv_style_set_text_align(&s_symbol_label_style, LV_TEXT_ALIGN_CENTER);
  lv_style_set_text_font(&s_symbol_label_style, &lv_font_montserrat_14);

  s_styles_ready = true;
}

void build_header(lv_obj_t *parent, lv_obj_t **time_label, lv_obj_t **wifi_label) {
  // Both home pages share the same header layout: logo on the left, time on top.
  lv_obj_t *logo = lv_img_create(parent);
  lv_obj_set_pos(logo, kHeaderLogoX, kHeaderLogoY);
  lv_img_set_src(logo, APP_IMG_LAFVIN);

  *time_label = lv_label_create(parent);
  lv_obj_set_pos(*time_label, kHeaderTimeX, kHeaderTimeY);
  lv_label_set_text(*time_label, "--:--");
  lv_obj_add_style(*time_label, &s_time_style, LV_PART_MAIN);

  *wifi_label = lv_label_create(parent);
  lv_obj_set_pos(*wifi_label, kHeaderWifiX, kHeaderWifiY);
  lv_label_set_text(*wifi_label, LV_SYMBOL_WIFI);
  lv_obj_add_style(*wifi_label, &s_wifi_style, LV_PART_MAIN);
  lv_obj_add_flag(*wifi_label, LV_OBJ_FLAG_HIDDEN);
}

lv_obj_t *create_icon_button(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, const lv_img_dsc_t *img, AppScreen screen) {
  // Each tile is a clickable image that opens one module screen.
  lv_obj_t *button = lv_img_create(parent);
  lv_obj_set_pos(button, x, y);
  lv_obj_set_size(button, kTileSize, kTileSize);
  lv_img_set_src(button, img);
  lv_obj_add_style(button, &s_icon_style, LV_PART_MAIN);
  lv_obj_add_style(button, &s_icon_pressed_style, LV_STATE_PRESSED);
  lv_obj_add_flag(button, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(button, app_icon_event_handler, LV_EVENT_CLICKED, reinterpret_cast<void *>(static_cast<uintptr_t>(screen)));
  return button;
}

lv_obj_t *create_symbol_button(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, const char *text, AppScreen screen) {
  lv_obj_t *button = lv_btn_create(parent);
  lv_obj_set_pos(button, x, y);
  lv_obj_set_size(button, kTileSize, kTileSize);
  lv_obj_add_style(button, &s_symbol_tile_style, LV_PART_MAIN);
  lv_obj_add_style(button, &s_symbol_tile_pressed_style, LV_STATE_PRESSED);
  lv_obj_add_event_cb(button, app_icon_event_handler, LV_EVENT_CLICKED, reinterpret_cast<void *>(static_cast<uintptr_t>(screen)));

  lv_obj_t *label = lv_label_create(button);
  lv_label_set_text(label, text);
  lv_obj_set_width(label, 64);
  lv_obj_center(label);
  lv_obj_add_style(label, &s_symbol_label_style, LV_PART_MAIN);

  return button;
}

void tileview_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
    return;
  }

  // Store the active page so returning from a child module restores the same page.
  lv_obj_t *active_tile = lv_tileview_get_tile_act(g_home_ui.tileview);
  if (active_tile == g_home_ui.page_two) {
    s_current_page = 1;
  } else {
    s_current_page = 0;
  }

  all_in_one_set_home_page(s_current_page);
}

void app_icon_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  const AppScreen target = static_cast<AppScreen>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
  all_in_one_open_screen(target);
}

}  // namespace

HomeUI g_home_ui = {};

void home_ui_setup(HomeUI *ui) {
  create_styles();

  // The screen itself is a plain white root object.
  ui->screen = lv_obj_create(nullptr);
  lv_obj_add_style(ui->screen, &s_screen_style, LV_PART_MAIN);
  lv_obj_clear_flag(ui->screen, LV_OBJ_FLAG_SCROLLABLE);

  // Use a tileview so the user can swipe horizontally between the two home pages.
  ui->tileview = lv_tileview_create(ui->screen);
  lv_obj_set_size(ui->tileview, kScreenWidth, kScreenHeight);
  lv_obj_center(ui->tileview);
  lv_obj_set_scrollbar_mode(ui->tileview, LV_SCROLLBAR_MODE_OFF);
  lv_obj_remove_style(ui->tileview, nullptr, LV_PART_SCROLLBAR | LV_STATE_ANY);
  lv_obj_set_style_bg_opa(ui->tileview, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui->tileview, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui->tileview, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(ui->tileview, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui->tileview, 0, LV_PART_MAIN);
  lv_obj_set_style_outline_width(ui->tileview, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui->tileview, LV_OPA_TRANSP, LV_PART_SCROLLBAR);
  lv_obj_set_style_border_opa(ui->tileview, LV_OPA_TRANSP, LV_PART_SCROLLBAR);
  lv_obj_set_style_shadow_width(ui->tileview, 0, LV_PART_SCROLLBAR);
  lv_obj_set_style_outline_width(ui->tileview, 0, LV_PART_SCROLLBAR);
  lv_obj_set_style_width(ui->tileview, 0, LV_PART_SCROLLBAR);
  lv_obj_set_style_pad_all(ui->tileview, 0, LV_PART_SCROLLBAR);
  lv_obj_add_event_cb(ui->tileview, tileview_event_handler, LV_EVENT_VALUE_CHANGED, nullptr);

  // Page 1: HeartRate, Camera, Music, WiFi, Buzzer, Gallery.
  ui->page_one = lv_tileview_add_tile(ui->tileview, 0, 0, LV_DIR_HOR);
  lv_obj_add_style(ui->page_one, &s_tile_style, LV_PART_MAIN);
  lv_obj_clear_flag(ui->page_one, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_style(ui->page_one, nullptr, LV_PART_SCROLLBAR | LV_STATE_ANY);
  lv_obj_set_style_bg_opa(ui->page_one, LV_OPA_TRANSP, LV_PART_SCROLLBAR);
  lv_obj_set_style_border_opa(ui->page_one, LV_OPA_TRANSP, LV_PART_SCROLLBAR);
  lv_obj_set_style_shadow_width(ui->page_one, 0, LV_PART_SCROLLBAR);
  lv_obj_set_style_outline_width(ui->page_one, 0, LV_PART_SCROLLBAR);
  lv_obj_set_style_width(ui->page_one, 0, LV_PART_SCROLLBAR);
  build_header(ui->page_one, &ui->time_label_page_one, &ui->wifi_label_page_one);

  for (uint8_t i = 0; i < 6; ++i) {
    create_icon_button(ui->page_one, kTileOneX[i], kTileOneY[i], kPageOneIcons[i], kPageOneTargets[i]);
  }

  // Page 2 exposes the remaining utility demos.
  ui->page_two = lv_tileview_add_tile(ui->tileview, 1, 0, LV_DIR_HOR);
  lv_obj_add_style(ui->page_two, &s_tile_style, LV_PART_MAIN);
  lv_obj_clear_flag(ui->page_two, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_style(ui->page_two, nullptr, LV_PART_SCROLLBAR | LV_STATE_ANY);
  lv_obj_set_style_bg_opa(ui->page_two, LV_OPA_TRANSP, LV_PART_SCROLLBAR);
  lv_obj_set_style_border_opa(ui->page_two, LV_OPA_TRANSP, LV_PART_SCROLLBAR);
  lv_obj_set_style_shadow_width(ui->page_two, 0, LV_PART_SCROLLBAR);
  lv_obj_set_style_outline_width(ui->page_two, 0, LV_PART_SCROLLBAR);
  lv_obj_set_style_width(ui->page_two, 0, LV_PART_SCROLLBAR);
  build_header(ui->page_two, &ui->time_label_page_two, &ui->wifi_label_page_two);
  create_icon_button(ui->page_two, kPageTwoIconX, kPageTwoIconY, APP_IMG_RGB, APP_SCREEN_RGB);
  create_symbol_button(ui->page_two, kPageTwoFileX, kPageTwoFileY, LV_SYMBOL_DIRECTORY "\nFiles", APP_SCREEN_FILE_MANAGER);

  home_ui_show_page(0, false);
}

void home_ui_show_page(uint8_t page_index, bool animate) {
  // Clamp the home screen to the two valid pages.
  s_current_page = page_index > 0 ? 1 : 0;
  lv_obj_t *target_tile = (s_current_page == 0) ? g_home_ui.page_one : g_home_ui.page_two;
  lv_obj_set_tile(g_home_ui.tileview, target_tile, animate ? LV_ANIM_ON : LV_ANIM_OFF);
  all_in_one_set_home_page(s_current_page);
}

void home_ui_set_time_text(const char *text) {
  // Fall back to a placeholder while NTP is not ready yet.
  const char *safe_text = (text != nullptr && text[0] != '\0') ? text : "--:--";
  if (g_home_ui.time_label_page_one != nullptr) {
    lv_label_set_text(g_home_ui.time_label_page_one, safe_text);
  }
  if (g_home_ui.time_label_page_two != nullptr) {
    lv_label_set_text(g_home_ui.time_label_page_two, safe_text);
  }
}

void home_ui_set_wifi_visible(bool visible) {
  if (g_home_ui.wifi_label_page_one != nullptr) {
    if (visible) {
      lv_obj_clear_flag(g_home_ui.wifi_label_page_one, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(g_home_ui.wifi_label_page_one, LV_OBJ_FLAG_HIDDEN);
    }
  }

  if (g_home_ui.wifi_label_page_two != nullptr) {
    if (visible) {
      lv_obj_clear_flag(g_home_ui.wifi_label_page_two, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(g_home_ui.wifi_label_page_two, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

uint8_t home_ui_get_page(void) {
  return s_current_page;
}
