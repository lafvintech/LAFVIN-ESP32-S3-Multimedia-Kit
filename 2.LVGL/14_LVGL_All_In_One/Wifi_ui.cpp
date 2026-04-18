#include "Wifi_ui.h"

#include "all_in_one_app.h"
#include "WiFi.h"

#include <cstring>
#include <stdint.h>

namespace {

constexpr uint8_t kMaxScanResults = 12;
constexpr uint32_t kConnectTimeoutMs = 15000;

char s_scanned_ssids[kMaxScanResults][33] = {};
int32_t s_scanned_rssi[kMaxScanResults] = {};
uint8_t s_scan_count = 0;

char s_selected_ssid[33] = {};
bool s_connecting = false;
uint32_t s_connect_started_ms = 0;

static lv_style_t s_screen_style;
static lv_style_t s_icon_button_style;
static lv_style_t s_icon_button_pressed_style;
static lv_style_t s_title_style;
static lv_style_t s_status_style;
static lv_style_t s_ip_style;
static lv_style_t s_overlay_style;
static lv_style_t s_dialog_style;
static lv_style_t s_dialog_title_style;
static lv_style_t s_action_button_style;
static lv_style_t s_action_button_pressed_style;
static bool s_styles_ready = false;

static void create_styles(void);
static void setup_header(WifiUI *ui);
static void setup_list(WifiUI *ui);
static void setup_footer(WifiUI *ui);
static void setup_dialog(WifiUI *ui);

static void scan_networks(void);
static void rebuild_wifi_list(void);
static void show_password_dialog(const char *ssid);
static void hide_password_dialog(void);
static void start_connection(void);
static void set_status_text(const char *text);
static void set_ip_text(const char *text);

static void exit_event_handler(lv_event_t *e);
static void refresh_event_handler(lv_event_t *e);
static void wifi_item_event_handler(lv_event_t *e);
static void connect_event_handler(lv_event_t *e);
static void cancel_event_handler(lv_event_t *e);
static void keyboard_event_handler(lv_event_t *e);

void create_styles(void) {
  if (s_styles_ready) {
    return;
  }

  lv_style_init(&s_screen_style);
  lv_style_set_bg_color(&s_screen_style, lv_color_hex(0xF8FAFC));
  lv_style_set_bg_opa(&s_screen_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_screen_style, 0);
  lv_style_set_pad_all(&s_screen_style, 0);

  lv_style_init(&s_icon_button_style);
  lv_style_set_bg_opa(&s_icon_button_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_icon_button_style, 0);
  lv_style_set_shadow_width(&s_icon_button_style, 0);
  lv_style_set_outline_width(&s_icon_button_style, 0);
  lv_style_set_text_color(&s_icon_button_style, lv_color_hex(0x2793E6));
  lv_style_set_text_font(&s_icon_button_style, &lv_font_montserrat_22);
  lv_style_set_pad_all(&s_icon_button_style, 0);

  lv_style_init(&s_icon_button_pressed_style);
  lv_style_set_translate_y(&s_icon_button_pressed_style, 2);
  lv_style_set_text_opa(&s_icon_button_pressed_style, LV_OPA_70);

  lv_style_init(&s_title_style);
  lv_style_set_text_color(&s_title_style, lv_color_hex(0x102A43));
  lv_style_set_text_font(&s_title_style, &lv_font_montserrat_22);

  lv_style_init(&s_status_style);
  lv_style_set_text_color(&s_status_style, lv_color_hex(0x334E68));
  lv_style_set_text_font(&s_status_style, &lv_font_montserrat_12);

  lv_style_init(&s_ip_style);
  lv_style_set_text_color(&s_ip_style, lv_color_hex(0x1F5F8B));
  lv_style_set_text_font(&s_ip_style, &lv_font_montserrat_14);

  lv_style_init(&s_overlay_style);
  lv_style_set_bg_color(&s_overlay_style, lv_color_hex(0x000000));
  lv_style_set_bg_opa(&s_overlay_style, LV_OPA_40);
  lv_style_set_border_width(&s_overlay_style, 0);
  lv_style_set_pad_all(&s_overlay_style, 0);

  lv_style_init(&s_dialog_style);
  lv_style_set_bg_color(&s_dialog_style, lv_color_white());
  lv_style_set_bg_opa(&s_dialog_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_dialog_style, 1);
  lv_style_set_border_color(&s_dialog_style, lv_color_hex(0xD9E2EC));
  lv_style_set_radius(&s_dialog_style, 16);
  lv_style_set_shadow_width(&s_dialog_style, 18);
  lv_style_set_shadow_opa(&s_dialog_style, LV_OPA_20);

  lv_style_init(&s_dialog_title_style);
  lv_style_set_text_color(&s_dialog_title_style, lv_color_hex(0x102A43));
  lv_style_set_text_font(&s_dialog_title_style, &lv_font_montserrat_16);

  lv_style_init(&s_action_button_style);
  lv_style_set_radius(&s_action_button_style, 12);
  lv_style_set_shadow_width(&s_action_button_style, 0);
  lv_style_set_border_width(&s_action_button_style, 0);
  lv_style_set_bg_color(&s_action_button_style, lv_color_hex(0x2793E6));
  lv_style_set_text_color(&s_action_button_style, lv_color_white());

  lv_style_init(&s_action_button_pressed_style);
  lv_style_set_translate_y(&s_action_button_pressed_style, 2);
  lv_style_set_bg_opa(&s_action_button_pressed_style, LV_OPA_80);

  s_styles_ready = true;
}

void setup_header(WifiUI *ui) {
  ui->exit_button = lv_btn_create(ui->screen);
  lv_obj_set_pos(ui->exit_button, 10, 8);
  lv_obj_set_size(ui->exit_button, 42, 32);
  lv_obj_add_style(ui->exit_button, &s_icon_button_style, LV_PART_MAIN);
  lv_obj_add_style(ui->exit_button, &s_icon_button_pressed_style, LV_STATE_PRESSED);
  lv_obj_add_event_cb(ui->exit_button, exit_event_handler, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *exit_label = lv_label_create(ui->exit_button);
  lv_label_set_text(exit_label, LV_SYMBOL_LEFT);
  lv_obj_center(exit_label);

  ui->refresh_button = lv_btn_create(ui->screen);
  lv_obj_set_pos(ui->refresh_button, 270, 8);
  lv_obj_set_size(ui->refresh_button, 42, 32);
  lv_obj_add_style(ui->refresh_button, &s_icon_button_style, LV_PART_MAIN);
  lv_obj_add_style(ui->refresh_button, &s_icon_button_pressed_style, LV_STATE_PRESSED);
  lv_obj_add_event_cb(ui->refresh_button, refresh_event_handler, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *refresh_label = lv_label_create(ui->refresh_button);
  lv_label_set_text(refresh_label, LV_SYMBOL_REFRESH);
  lv_obj_center(refresh_label);

  ui->title_label = lv_label_create(ui->screen);
  lv_obj_set_pos(ui->title_label, 64, 10);
  lv_label_set_text(ui->title_label, "WiFi");
  lv_obj_add_style(ui->title_label, &s_title_style, LV_PART_MAIN);
}

void setup_list(WifiUI *ui) {
  ui->wifi_list = lv_list_create(ui->screen);
  lv_obj_set_pos(ui->wifi_list, 10, 44);
  lv_obj_set_size(ui->wifi_list, 300, 140);
  lv_obj_set_style_radius(ui->wifi_list, 14, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui->wifi_list, 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(ui->wifi_list, lv_color_hex(0xD9E2EC), LV_PART_MAIN);
  lv_obj_set_style_pad_row(ui->wifi_list, 6, LV_PART_MAIN);
  lv_obj_set_style_pad_top(ui->wifi_list, 6, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(ui->wifi_list, 6, LV_PART_MAIN);
}

void setup_footer(WifiUI *ui) {
  ui->status_label = lv_label_create(ui->screen);
  lv_obj_set_pos(ui->status_label, 12, 190);
  lv_obj_set_width(ui->status_label, 296);
  lv_label_set_text(ui->status_label, "Status: Scanning nearby WiFi...");
  lv_obj_add_style(ui->status_label, &s_status_style, LV_PART_MAIN);

  ui->ip_label = lv_label_create(ui->screen);
  lv_obj_set_pos(ui->ip_label, 12, 212);
  lv_obj_set_width(ui->ip_label, 296);
  lv_label_set_text(ui->ip_label, "IP: --");
  lv_obj_add_style(ui->ip_label, &s_ip_style, LV_PART_MAIN);
}

void setup_dialog(WifiUI *ui) {
  ui->overlay = lv_obj_create(ui->screen);
  lv_obj_set_size(ui->overlay, 320, 240);
  lv_obj_add_style(ui->overlay, &s_overlay_style, LV_PART_MAIN);
  lv_obj_clear_flag(ui->overlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(ui->overlay, LV_OBJ_FLAG_HIDDEN);

  ui->dialog = lv_obj_create(ui->overlay);
  lv_obj_set_pos(ui->dialog, 10, 6);
  lv_obj_set_size(ui->dialog, 300, 35);
  lv_obj_add_style(ui->dialog, &s_dialog_style, LV_PART_MAIN);
  lv_obj_clear_flag(ui->dialog, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(ui->dialog, 0, LV_PART_MAIN);

  ui->dialog_title = lv_label_create(ui->dialog);
  lv_obj_add_flag(ui->dialog_title, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_style(ui->dialog_title, &s_dialog_title_style, LV_PART_MAIN);

  ui->password_area = lv_textarea_create(ui->dialog);
  lv_obj_set_pos(ui->password_area, 12, 0);
  lv_obj_set_size(ui->password_area, 144, 22);
  lv_textarea_set_one_line(ui->password_area, true);
  lv_textarea_set_password_mode(ui->password_area, true);
  lv_textarea_set_placeholder_text(ui->password_area, "Enter password");
  lv_obj_set_style_radius(ui->password_area, 10, LV_PART_MAIN);

  ui->cancel_button = lv_btn_create(ui->dialog);
  lv_obj_set_pos(ui->cancel_button, 164, 5);
  lv_obj_set_size(ui->cancel_button, 58, 22);
  lv_obj_add_style(ui->cancel_button, &s_action_button_style, LV_PART_MAIN);
  lv_obj_add_style(ui->cancel_button, &s_action_button_pressed_style, LV_STATE_PRESSED);
  lv_obj_set_style_bg_color(ui->cancel_button, lv_color_hex(0x9FB3C8), LV_PART_MAIN);
  lv_obj_add_event_cb(ui->cancel_button, cancel_event_handler, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *cancel_label = lv_label_create(ui->cancel_button);
  lv_label_set_text(cancel_label, "Back");
  lv_obj_center(cancel_label);

  ui->connect_button = lv_btn_create(ui->dialog);
  lv_obj_set_pos(ui->connect_button, 230, 5);
  lv_obj_set_size(ui->connect_button, 58, 22);
  lv_obj_add_style(ui->connect_button, &s_action_button_style, LV_PART_MAIN);
  lv_obj_add_style(ui->connect_button, &s_action_button_pressed_style, LV_STATE_PRESSED);
  lv_obj_add_event_cb(ui->connect_button, connect_event_handler, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *connect_label = lv_label_create(ui->connect_button);
  lv_label_set_text(connect_label, "Join");
  lv_obj_center(connect_label);

  ui->keyboard = lv_keyboard_create(ui->overlay);
  lv_obj_set_pos(ui->keyboard, 0, 10);
  lv_obj_set_size(ui->keyboard, 320, 160);
  lv_keyboard_set_mode(ui->keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
  lv_keyboard_set_popovers(ui->keyboard, false);
  lv_keyboard_set_textarea(ui->keyboard, ui->password_area);
  lv_obj_add_event_cb(ui->keyboard, keyboard_event_handler, LV_EVENT_ALL, nullptr);
}

void set_status_text(const char *text) {
  lv_label_set_text(g_wifi_ui.status_label, text);
}

void set_ip_text(const char *text) {
  lv_label_set_text(g_wifi_ui.ip_label, text);
}

void rebuild_wifi_list(void) {
  lv_obj_clean(g_wifi_ui.wifi_list);

  if (s_scan_count == 0) {
    lv_obj_t *label = lv_label_create(g_wifi_ui.wifi_list);
    lv_label_set_text(label, "No WiFi found. Tap refresh to scan again.");
    return;
  }

  for (uint8_t i = 0; i < s_scan_count; ++i) {
    lv_obj_t *btn = lv_list_add_btn(g_wifi_ui.wifi_list, LV_SYMBOL_WIFI, s_scanned_ssids[i]);
    lv_obj_add_event_cb(
      btn,
      wifi_item_event_handler,
      LV_EVENT_CLICKED,
      reinterpret_cast<void *>(static_cast<uintptr_t>(i)));

    lv_obj_t *btn_label = lv_obj_get_child(btn, 1);
    if (btn_label != nullptr) {
      lv_label_set_text_fmt(btn_label, "%s  (%d dBm)", s_scanned_ssids[i], static_cast<int>(s_scanned_rssi[i]));
    }
  }
}

void scan_networks(void) {
  set_status_text("Status: Scanning nearby WiFi...");
  set_ip_text("IP: --");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(50);

  const int found = WiFi.scanNetworks(false, true);
  s_scan_count = 0;

  if (found <= 0) {
    rebuild_wifi_list();
    set_status_text("Status: No WiFi found");
    return;
  }

  const int limited_count = (found > kMaxScanResults) ? kMaxScanResults : found;
  for (int i = 0; i < limited_count; ++i) {
    const String ssid = WiFi.SSID(i);
    ssid.toCharArray(s_scanned_ssids[i], sizeof(s_scanned_ssids[i]));
    s_scanned_rssi[i] = WiFi.RSSI(i);
    s_scan_count++;
  }

  WiFi.scanDelete();
  rebuild_wifi_list();

  char buffer[64];
  snprintf(buffer, sizeof(buffer), "Status: Found %u network(s)", static_cast<unsigned>(s_scan_count));
  set_status_text(buffer);
}

void show_password_dialog(const char *ssid) {
  strncpy(s_selected_ssid, ssid, sizeof(s_selected_ssid) - 1);
  s_selected_ssid[sizeof(s_selected_ssid) - 1] = '\0';

  lv_textarea_set_text(g_wifi_ui.password_area, "");
  lv_obj_clear_flag(g_wifi_ui.overlay, LV_OBJ_FLAG_HIDDEN);
  lv_keyboard_set_textarea(g_wifi_ui.keyboard, g_wifi_ui.password_area);
  lv_obj_move_foreground(g_wifi_ui.overlay);
}

void hide_password_dialog(void) {
  lv_obj_add_flag(g_wifi_ui.overlay, LV_OBJ_FLAG_HIDDEN);
}

void start_connection(void) {
  hide_password_dialog();

  const char *password = lv_textarea_get_text(g_wifi_ui.password_area);
  WiFi.disconnect();
  WiFi.begin(s_selected_ssid, password);

  s_connecting = true;
  s_connect_started_ms = millis();

  char buffer[96];
  snprintf(buffer, sizeof(buffer), "Status: Connecting to %s ...", s_selected_ssid);
  set_status_text(buffer);
  set_ip_text("IP: --");
}

void exit_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  hide_password_dialog();
  all_in_one_show_home();
}

void refresh_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  scan_networks();
}

void wifi_item_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  const uint8_t index = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
  if (index >= s_scan_count) {
    return;
  }

  show_password_dialog(s_scanned_ssids[index]);
}

void connect_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  start_connection();
}

void cancel_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  hide_password_dialog();
}

void keyboard_event_handler(lv_event_t *e) {
  const lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CANCEL) {
    hide_password_dialog();
    return;
  }

  if (code == LV_EVENT_READY) {
    start_connection();
  }
}

}  // namespace

WifiUI g_wifi_ui = {};

void wifi_ui_setup(WifiUI *ui) {
  create_styles();

  ui->screen = lv_obj_create(nullptr);
  lv_obj_add_style(ui->screen, &s_screen_style, LV_PART_MAIN);
  lv_obj_clear_flag(ui->screen, LV_OBJ_FLAG_SCROLLABLE);

  setup_header(ui);
  setup_list(ui);
  setup_footer(ui);
  setup_dialog(ui);

  scan_networks();
  Serial.println("WiFi UI ready.");
}

void wifi_ui_loop(void) {
  if (!s_connecting) {
    return;
  }

  const wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    s_connecting = false;
    char status_buffer[96];
    snprintf(status_buffer, sizeof(status_buffer), "Status: Connected to %s", s_selected_ssid);
    set_status_text(status_buffer);
    all_in_one_notify_wifi_connected();

    const String ip = WiFi.localIP().toString();
    char ip_buffer[64];
    snprintf(ip_buffer, sizeof(ip_buffer), "IP: %s", ip.c_str());
    set_ip_text(ip_buffer);
    Serial.println(status_buffer);
    Serial.println(ip_buffer);
    return;
  }

  if (millis() - s_connect_started_ms >= kConnectTimeoutMs ||
      status == WL_CONNECT_FAILED ||
      status == WL_NO_SSID_AVAIL) {
    s_connecting = false;
    set_status_text("Status: Connection failed");
    set_ip_text("IP: --");
    WiFi.disconnect();
    Serial.println("WiFi connection failed.");
  }
}
