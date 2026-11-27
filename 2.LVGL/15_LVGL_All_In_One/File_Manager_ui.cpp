#include "File_Manager_ui.h"

#include "all_in_one_app.h"
#include "WiFi.h"
#include "WebServer.h"
#include "img/img_index.h"
#include "sd_card.h"

#include <cstdio>
#include <cstring>

namespace {

constexpr uint8_t kMaxEntries = 48;
constexpr size_t kMaxPathLength = 192;
constexpr size_t kMaxNameLength = 80;
constexpr uint8_t kParentEntryIndex = 255;
constexpr uint32_t kWifiPollIntervalMs = 600;

struct FileEntry {
  char name[kMaxNameLength];
  char path[kMaxPathLength];
  bool is_dir;
  size_t size_bytes;
};

FileEntry s_entries[kMaxEntries] = {};
uint8_t s_entry_count = 0;

char s_current_path[kMaxPathLength] = "/";
char s_selected_file_path[kMaxPathLength] = {};
char s_selected_file_name[kMaxNameLength] = {};

WebServer s_server(80);
bool s_routes_ready = false;
bool s_server_started = false;
bool s_remote_ready = false;
uint32_t s_last_wifi_poll_ms = 0;

File s_upload_file;
String s_upload_directory = "/";
String s_upload_path;
String s_upload_temp_path;
String s_upload_error;
size_t s_upload_written = 0;
size_t s_upload_expected = 0;
bool s_upload_has_expected = false;
bool s_upload_in_progress = false;
bool s_upload_ok = false;
bool s_upload_result_ready = false;
bool s_upload_owns_pause = false;
bool s_file_list_dirty = false;
String s_last_remote_message;

static lv_style_t s_screen_style;
static lv_style_t s_icon_button_style;
static lv_style_t s_icon_button_pressed_style;
static lv_style_t s_title_style;
static lv_style_t s_path_style;
static lv_style_t s_status_style;
static lv_style_t s_list_style;
static lv_style_t s_overlay_style;
static lv_style_t s_dialog_style;
static lv_style_t s_dialog_title_style;
static lv_style_t s_dialog_text_style;
static lv_style_t s_action_button_style;
static lv_style_t s_action_button_pressed_style;
static bool s_styles_ready = false;

static void create_styles(void);
static void build_header(FileManagerUI *ui);
static void build_file_list(FileManagerUI *ui);
static void build_footer(FileManagerUI *ui);
static void build_dialog(FileManagerUI *ui);

static void refresh_file_list(void);
static void update_path_label(void);
static void update_status_label(void);
static void hide_dialog(void);
static void show_file_dialog(uint8_t entry_index);
static bool navigate_to(const char *path);
static bool navigate_to_parent(void);

static String basename_from_path(const String &full_path);
static void copy_string(char *dest, size_t dest_size, const String &value);
static void format_size(size_t size_bytes, char *buffer, size_t buffer_size);
static void update_delete_button_state(bool enabled);
static String normalize_path(const String &raw_path, bool directory_path);
static String mounted_path(const String &path);
static String join_path(const String &dir, const String &name);
static String parent_path(const String &path);
static String url_decode(const String &input);
static String url_encode(const String &input);
static String html_escape(const String &input);
static String remote_url(void);
static File open_path_for_read(const String &path);
static File open_path_for_write(const String &path);
static bool remove_file_path(const String &path);
static bool remove_dir_path(const String &path);
static bool path_exists(const String &path);

static void ensure_server_state(void);
static void start_remote_server(void);
static void stop_remote_server(void);
static void configure_server_routes(void);
static void redirect_to_directory(const String &dir);

static void handle_root_page(void);
static void handle_download(void);
static void handle_delete(void);
static void handle_mkdir(void);
static void handle_upload_finish(void);
static void handle_upload_stream(void);
static void handle_not_found(void);
static void finish_upload(bool success, const char *message);
static bool commit_upload(void);

static void exit_event_handler(lv_event_t *e);
static void refresh_event_handler(lv_event_t *e);
static void file_item_event_handler(lv_event_t *e);
static void cancel_event_handler(lv_event_t *e);
static void delete_event_handler(lv_event_t *e);

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
  lv_style_set_text_font(&s_icon_button_style, &lv_font_montserrat_20);
  lv_style_set_pad_all(&s_icon_button_style, 0);

  lv_style_init(&s_icon_button_pressed_style);
  lv_style_set_translate_y(&s_icon_button_pressed_style, 2);
  lv_style_set_text_opa(&s_icon_button_pressed_style, LV_OPA_70);

  lv_style_init(&s_title_style);
  lv_style_set_text_color(&s_title_style, lv_color_hex(0x102A43));
  lv_style_set_text_font(&s_title_style, &lv_font_montserrat_20);

  lv_style_init(&s_path_style);
  lv_style_set_text_color(&s_path_style, lv_color_hex(0x486581));
  lv_style_set_text_font(&s_path_style, &lv_font_montserrat_12);

  lv_style_init(&s_status_style);
  lv_style_set_text_color(&s_status_style, lv_color_hex(0x334E68));
  lv_style_set_text_font(&s_status_style, &lv_font_montserrat_12);

  lv_style_init(&s_list_style);
  lv_style_set_radius(&s_list_style, 14);
  lv_style_set_border_width(&s_list_style, 1);
  lv_style_set_border_color(&s_list_style, lv_color_hex(0xD9E2EC));
  lv_style_set_pad_row(&s_list_style, 6);
  lv_style_set_pad_top(&s_list_style, 6);
  lv_style_set_pad_bottom(&s_list_style, 6);
  lv_style_set_bg_color(&s_list_style, lv_color_white());

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
  lv_style_set_shadow_width(&s_dialog_style, 16);
  lv_style_set_shadow_opa(&s_dialog_style, LV_OPA_20);

  lv_style_init(&s_dialog_title_style);
  lv_style_set_text_color(&s_dialog_title_style, lv_color_hex(0x102A43));
  lv_style_set_text_font(&s_dialog_title_style, &lv_font_montserrat_16);

  lv_style_init(&s_dialog_text_style);
  lv_style_set_text_color(&s_dialog_text_style, lv_color_hex(0x334E68));
  lv_style_set_text_font(&s_dialog_text_style, &lv_font_montserrat_12);

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

void build_header(FileManagerUI *ui) {
  ui->exit_button = lv_btn_create(ui->screen);
  lv_obj_set_pos(ui->exit_button, 10, 8);
  lv_obj_set_size(ui->exit_button, 42, 32);
  lv_obj_add_style(ui->exit_button, &s_icon_button_style, LV_PART_MAIN);
  lv_obj_add_style(ui->exit_button, &s_icon_button_pressed_style, LV_STATE_PRESSED);
  lv_obj_add_event_cb(ui->exit_button, exit_event_handler, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *exit_img = lv_img_create(ui->exit_button);
  lv_img_set_src(exit_img, APP_IMG_ESC);
  lv_obj_center(exit_img);

  ui->refresh_button = lv_btn_create(ui->screen);
  lv_obj_set_pos(ui->refresh_button, 232, 8);
  lv_obj_set_size(ui->refresh_button, 36, 32);
  lv_obj_add_style(ui->refresh_button, &s_icon_button_style, LV_PART_MAIN);
  lv_obj_add_style(ui->refresh_button, &s_icon_button_pressed_style, LV_STATE_PRESSED);
  lv_obj_add_event_cb(ui->refresh_button, refresh_event_handler, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *refresh_label = lv_label_create(ui->refresh_button);
  lv_label_set_text(refresh_label, LV_SYMBOL_REFRESH);
  lv_obj_center(refresh_label);

  ui->wifi_badge = lv_btn_create(ui->screen);
  lv_obj_set_pos(ui->wifi_badge, 272, 8);
  lv_obj_set_size(ui->wifi_badge, 36, 32);
  lv_obj_add_style(ui->wifi_badge, &s_icon_button_style, LV_PART_MAIN);
  lv_obj_add_style(ui->wifi_badge, &s_icon_button_pressed_style, LV_STATE_PRESSED);
  lv_obj_clear_flag(ui->wifi_badge, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(ui->wifi_badge, LV_OBJ_FLAG_HIDDEN);
  lv_obj_t *wifi_label = lv_label_create(ui->wifi_badge);
  lv_label_set_text(wifi_label, LV_SYMBOL_WIFI);
  lv_obj_center(wifi_label);

  ui->title_label = lv_label_create(ui->screen);
  lv_obj_set_pos(ui->title_label, 64, 10);
  lv_label_set_text(ui->title_label, "Files");
  lv_obj_add_style(ui->title_label, &s_title_style, LV_PART_MAIN);

  ui->path_label = lv_label_create(ui->screen);
  lv_obj_set_pos(ui->path_label, 12, 42);
  lv_obj_set_width(ui->path_label, 296);
  lv_label_set_long_mode(ui->path_label, LV_LABEL_LONG_DOT);
  lv_obj_add_style(ui->path_label, &s_path_style, LV_PART_MAIN);
}

void build_file_list(FileManagerUI *ui) {
  ui->list = lv_list_create(ui->screen);
  lv_obj_set_pos(ui->list, 10, 62);
  lv_obj_set_size(ui->list, 300, 140);
  lv_obj_add_style(ui->list, &s_list_style, LV_PART_MAIN);
}

void build_footer(FileManagerUI *ui) {
  ui->status_label = lv_label_create(ui->screen);
  lv_obj_set_pos(ui->status_label, 12, 208);
  lv_obj_set_width(ui->status_label, 296);
  lv_label_set_long_mode(ui->status_label, LV_LABEL_LONG_WRAP);
  lv_label_set_text(ui->status_label, "Loading files...");
  lv_obj_add_style(ui->status_label, &s_status_style, LV_PART_MAIN);
}

void build_dialog(FileManagerUI *ui) {
  ui->overlay = lv_obj_create(ui->screen);
  lv_obj_set_size(ui->overlay, 320, 240);
  lv_obj_add_style(ui->overlay, &s_overlay_style, LV_PART_MAIN);
  lv_obj_clear_flag(ui->overlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(ui->overlay, LV_OBJ_FLAG_HIDDEN);

  ui->dialog = lv_obj_create(ui->overlay);
  lv_obj_set_pos(ui->dialog, 24, 44);
  lv_obj_set_size(ui->dialog, 272, 122);
  lv_obj_add_style(ui->dialog, &s_dialog_style, LV_PART_MAIN);
  lv_obj_clear_flag(ui->dialog, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(ui->dialog, 12, LV_PART_MAIN);

  ui->dialog_title = lv_label_create(ui->dialog);
  lv_obj_set_width(ui->dialog_title, 248);
  lv_label_set_text(ui->dialog_title, "File");
  lv_obj_add_style(ui->dialog_title, &s_dialog_title_style, LV_PART_MAIN);

  ui->dialog_info = lv_label_create(ui->dialog);
  lv_obj_set_pos(ui->dialog_info, 0, 28);
  lv_obj_set_width(ui->dialog_info, 248);
  lv_label_set_long_mode(ui->dialog_info, LV_LABEL_LONG_WRAP);
  lv_label_set_text(ui->dialog_info, "");
  lv_obj_add_style(ui->dialog_info, &s_dialog_text_style, LV_PART_MAIN);

  ui->cancel_button = lv_btn_create(ui->dialog);
  lv_obj_set_pos(ui->cancel_button, 82, 82);
  lv_obj_set_size(ui->cancel_button, 72, 28);
  lv_obj_add_style(ui->cancel_button, &s_action_button_style, LV_PART_MAIN);
  lv_obj_add_style(ui->cancel_button, &s_action_button_pressed_style, LV_STATE_PRESSED);
  lv_obj_set_style_bg_color(ui->cancel_button, lv_color_hex(0x9FB3C8), LV_PART_MAIN);
  lv_obj_add_event_cb(ui->cancel_button, cancel_event_handler, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *cancel_label = lv_label_create(ui->cancel_button);
  lv_label_set_text(cancel_label, "Close");
  lv_obj_center(cancel_label);

  ui->delete_button = lv_btn_create(ui->dialog);
  lv_obj_set_pos(ui->delete_button, 168, 82);
  lv_obj_set_size(ui->delete_button, 72, 28);
  lv_obj_add_style(ui->delete_button, &s_action_button_style, LV_PART_MAIN);
  lv_obj_add_style(ui->delete_button, &s_action_button_pressed_style, LV_STATE_PRESSED);
  lv_obj_set_style_bg_color(ui->delete_button, lv_color_hex(0xD64545), LV_PART_MAIN);
  lv_obj_add_event_cb(ui->delete_button, delete_event_handler, LV_EVENT_CLICKED, nullptr);
  lv_obj_t *delete_label = lv_label_create(ui->delete_button);
  lv_label_set_text(delete_label, "Delete");
  lv_obj_center(delete_label);
}

void update_path_label(void) {
  lv_label_set_text_fmt(g_file_manager_ui.path_label, "Path: %s", s_current_path);
}

void hide_dialog(void) {
  lv_obj_add_flag(g_file_manager_ui.overlay, LV_OBJ_FLAG_HIDDEN);
  s_selected_file_path[0] = '\0';
  s_selected_file_name[0] = '\0';
}

String basename_from_path(const String &full_path) {
  const int slash = full_path.lastIndexOf('/');
  if (slash < 0) {
    return full_path;
  }
  return full_path.substring(slash + 1);
}

void copy_string(char *dest, size_t dest_size, const String &value) {
  if (dest == nullptr || dest_size == 0) {
    return;
  }

  const size_t copy_length = value.length() < (dest_size - 1) ? value.length() : (dest_size - 1);
  memcpy(dest, value.c_str(), copy_length);
  dest[copy_length] = '\0';
}

void format_size(size_t size_bytes, char *buffer, size_t buffer_size) {
  if (buffer == nullptr || buffer_size == 0) {
    return;
  }

  if (size_bytes >= 1024UL * 1024UL) {
    snprintf(buffer, buffer_size, "%.1f MB", static_cast<double>(size_bytes) / (1024.0 * 1024.0));
  } else if (size_bytes >= 1024UL) {
    snprintf(buffer, buffer_size, "%.1f KB", static_cast<double>(size_bytes) / 1024.0);
  } else {
    snprintf(buffer, buffer_size, "%u B", static_cast<unsigned>(size_bytes));
  }
}

void update_delete_button_state(bool enabled) {
  if (enabled) {
    lv_obj_clear_state(g_file_manager_ui.delete_button, LV_STATE_DISABLED);
  } else {
    lv_obj_add_state(g_file_manager_ui.delete_button, LV_STATE_DISABLED);
  }
}

String normalize_path(const String &raw_path, bool directory_path) {
  String path = raw_path;
  path.replace('\\', '/');
  if (path.startsWith("/sdcard")) {
    path.remove(0, 7);
  } else if (path.startsWith("sdcard/")) {
    path = path.substring(6);
  }
  if (path.isEmpty()) {
    path = "/";
  }
  if (!path.startsWith("/")) {
    path = "/" + path;
  }
  while (path.indexOf("//") >= 0) {
    path.replace("//", "/");
  }
  if (path.indexOf("..") >= 0) {
    return "/";
  }
  if (directory_path && path.length() > 1 && path.endsWith("/")) {
    path.remove(path.length() - 1);
  }
  return path;
}

String mounted_path(const String &path) {
  const String normalized = normalize_path(path, false);
  if (normalized == "/") {
    return "/sdcard";
  }
  return String("/sdcard") + normalized;
}

String join_path(const String &dir, const String &name) {
  const String safe_dir = normalize_path(dir, true);
  if (safe_dir == "/") {
    return normalize_path("/" + name, false);
  }
  return normalize_path(safe_dir + "/" + name, false);
}

String parent_path(const String &path) {
  const String normalized = normalize_path(path, true);
  if (normalized == "/") {
    return "/";
  }

  const int slash = normalized.lastIndexOf('/');
  if (slash <= 0) {
    return "/";
  }
  return normalized.substring(0, slash);
}

String url_decode(const String &input) {
  String decoded;
  decoded.reserve(input.length());

  for (size_t i = 0; i < input.length(); ++i) {
    const char ch = input[i];
    if (ch == '+') {
      decoded += ' ';
    } else if (ch == '%' && i + 2 < input.length()) {
      char hex[3] = {input[i + 1], input[i + 2], '\0'};
      decoded += static_cast<char>(strtol(hex, nullptr, 16));
      i += 2;
    } else {
      decoded += ch;
    }
  }

  return decoded;
}

String url_encode(const String &input) {
  const char *hex = "0123456789ABCDEF";
  String encoded;
  encoded.reserve(input.length() * 3);

  for (size_t i = 0; i < input.length(); ++i) {
    const unsigned char ch = static_cast<unsigned char>(input[i]);
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
        (ch >= '0' && ch <= '9') || ch == '-' || ch == '_' ||
        ch == '.' || ch == '~' || ch == '/') {
      encoded += static_cast<char>(ch);
    } else if (ch == ' ') {
      encoded += '+';
    } else {
      encoded += '%';
      encoded += hex[(ch >> 4) & 0x0F];
      encoded += hex[ch & 0x0F];
    }
  }

  return encoded;
}

String html_escape(const String &input) {
  String escaped = input;
  escaped.replace("&", "&amp;");
  escaped.replace("<", "&lt;");
  escaped.replace(">", "&gt;");
  escaped.replace("\"", "&quot;");
  return escaped;
}

String remote_url(void) {
  if (!s_remote_ready) {
    return String();
  }
  return String("http://") + WiFi.localIP().toString() + "/";
}

File open_path_for_read(const String &path) {
  const String normalized = normalize_path(path, false);
  File file = SD_MMC.open(normalized.c_str());
  if (file) {
    return file;
  }

  const String prefixed = mounted_path(normalized);
  return SD_MMC.open(prefixed.c_str());
}

File open_path_for_write(const String &path) {
  const String normalized = normalize_path(path, false);
  File file = SD_MMC.open(normalized.c_str(), FILE_WRITE);
  if (file) {
    return file;
  }

  const String prefixed = mounted_path(normalized);
  return SD_MMC.open(prefixed.c_str(), FILE_WRITE);
}

bool remove_file_path(const String &path) {
  const String normalized = normalize_path(path, false);
  if (SD_MMC.remove(normalized.c_str())) {
    return true;
  }

  const String prefixed = mounted_path(normalized);
  return SD_MMC.remove(prefixed.c_str());
}

bool remove_dir_path(const String &path) {
  const String normalized = normalize_path(path, true);
  if (SD_MMC.rmdir(normalized.c_str())) {
    return true;
  }

  const String prefixed = mounted_path(normalized);
  return SD_MMC.rmdir(prefixed.c_str());
}

bool path_exists(const String &path) {
  const String normalized = normalize_path(path, false);
  if (SD_MMC.exists(normalized.c_str())) {
    return true;
  }

  const String prefixed = mounted_path(normalized);
  return SD_MMC.exists(prefixed.c_str());
}

bool navigate_to(const char *path) {
  if (path == nullptr || path[0] == '\0') {
    return false;
  }

  const String normalized = normalize_path(path, true);
  File dir = open_path_for_read(normalized);
  if (!dir || !dir.isDirectory()) {
    if (dir) {
      dir.close();
    }
    return false;
  }

  copy_string(s_current_path, sizeof(s_current_path), normalized);
  dir.close();
  refresh_file_list();
  return true;
}

bool navigate_to_parent(void) {
  const String parent = parent_path(String(s_current_path));
  return navigate_to(parent.c_str());
}

void update_status_label(void) {
  char storage_text[32];
  const uint64_t total_mb = SD_MMC.totalBytes() / (1024ULL * 1024ULL);
  const uint64_t used_mb = SD_MMC.usedBytes() / (1024ULL * 1024ULL);
  snprintf(storage_text, sizeof(storage_text), "%llu / %llu MB", used_mb, total_mb);

  const String remote = remote_url();
  if (remote.length() > 0) {
    lv_label_set_text_fmt(
      g_file_manager_ui.status_label,
      "Items: %u  |  Storage: %s\nLAN: %s",
      static_cast<unsigned>(s_entry_count),
      storage_text,
      remote.c_str());
  } else {
    lv_label_set_text_fmt(
      g_file_manager_ui.status_label,
      "Items: %u  |  Storage: %s\nLAN: Connect WiFi to enable browser access",
      static_cast<unsigned>(s_entry_count),
      storage_text);
  }
}

void refresh_file_list(void) {
  update_path_label();
  lv_obj_clean(g_file_manager_ui.list);

  File dir = open_path_for_read(String(s_current_path));
  if (!dir || !dir.isDirectory()) {
    if (dir) {
      dir.close();
    }
    copy_string(s_current_path, sizeof(s_current_path), String("/"));
    update_path_label();
    dir = open_path_for_read(String(s_current_path));
    if (!dir || !dir.isDirectory()) {
      if (dir) {
        dir.close();
      }
      lv_label_set_text(g_file_manager_ui.status_label, "Unable to open the SD card root directory.");
      return;
    }
  }

  s_entry_count = 0;

  if (strcmp(s_current_path, "/") != 0) {
    lv_obj_t *parent_btn = lv_list_add_btn(g_file_manager_ui.list, LV_SYMBOL_LEFT, "..");
    lv_obj_add_event_cb(
      parent_btn,
      file_item_event_handler,
      LV_EVENT_CLICKED,
      reinterpret_cast<void *>(static_cast<uintptr_t>(kParentEntryIndex)));
  }

  File file = dir.openNextFile();
  while (file && s_entry_count < kMaxEntries) {
    // file.name() may return either a bare basename or a full path depending on
    // the ESP32 Arduino core version. Always re-join against the current dir so
    // the stored path is guaranteed to point at the real file.
    const String raw_name = String(file.name());
    const String entry_name = basename_from_path(normalize_path(raw_name, false));
    const String full_path = join_path(String(s_current_path), entry_name);

    copy_string(s_entries[s_entry_count].name, sizeof(s_entries[s_entry_count].name), entry_name);
    copy_string(s_entries[s_entry_count].path, sizeof(s_entries[s_entry_count].path), full_path);
    s_entries[s_entry_count].is_dir = file.isDirectory();
    s_entries[s_entry_count].size_bytes = static_cast<size_t>(file.size());

    const char *symbol = s_entries[s_entry_count].is_dir ? LV_SYMBOL_DIRECTORY : LV_SYMBOL_FILE;
    lv_obj_t *btn = lv_list_add_btn(g_file_manager_ui.list, symbol, s_entries[s_entry_count].name);
    lv_obj_add_event_cb(
      btn,
      file_item_event_handler,
      LV_EVENT_CLICKED,
      reinterpret_cast<void *>(static_cast<uintptr_t>(s_entry_count)));

    if (!s_entries[s_entry_count].is_dir) {
      lv_obj_t *btn_label = lv_obj_get_child(btn, 1);
      if (btn_label != nullptr) {
        char size_text[24];
        format_size(s_entries[s_entry_count].size_bytes, size_text, sizeof(size_text));
        lv_label_set_text_fmt(btn_label, "%s  (%s)", s_entries[s_entry_count].name, size_text);
      }
    }

    ++s_entry_count;
    file.close();
    file = dir.openNextFile();
  }

  dir.close();

  if (s_entry_count == 0 && strcmp(s_current_path, "/") == 0) {
    lv_obj_t *label = lv_label_create(g_file_manager_ui.list);
    lv_label_set_text(label, "The SD card is empty.");
  }

  update_status_label();
}

void show_file_dialog(uint8_t entry_index) {
  if (entry_index >= s_entry_count) {
    return;
  }

  copy_string(s_selected_file_path, sizeof(s_selected_file_path), String(s_entries[entry_index].path));
  copy_string(s_selected_file_name, sizeof(s_selected_file_name), String(s_entries[entry_index].name));

  char size_text[24];
  format_size(s_entries[entry_index].size_bytes, size_text, sizeof(size_text));

  lv_label_set_text(g_file_manager_ui.dialog_title, s_selected_file_name);
  lv_label_set_text_fmt(
    g_file_manager_ui.dialog_info,
    "Type: %s\nSize: %s\nPath: %s",
    s_entries[entry_index].is_dir ? "Folder" : "File",
    s_entries[entry_index].is_dir ? "--" : size_text,
    s_entries[entry_index].path);

  update_delete_button_state(!s_entries[entry_index].is_dir);
  lv_obj_clear_flag(g_file_manager_ui.overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(g_file_manager_ui.overlay);
}

void configure_server_routes(void) {
  if (s_routes_ready) {
    return;
  }

  s_server.on("/", HTTP_GET, handle_root_page);
  s_server.on("/download", HTTP_GET, handle_download);
  s_server.on("/delete", HTTP_GET, handle_delete);
  s_server.on("/mkdir", HTTP_GET, handle_mkdir);
  s_server.on("/upload", HTTP_POST, handle_upload_finish, handle_upload_stream);
  s_server.onNotFound(handle_not_found);

  s_routes_ready = true;
}

void start_remote_server(void) {
  if (s_server_started) {
    return;
  }

  configure_server_routes();
  s_server.begin();
  s_server_started = true;
  s_remote_ready = true;
  s_last_remote_message = String("Remote manager ready at ") + remote_url();
  Serial.println(s_last_remote_message);
}

void stop_remote_server(void) {
  if (!s_server_started) {
    s_remote_ready = false;
    return;
  }

  s_server.stop();
  s_server_started = false;
  s_remote_ready = false;
  s_last_remote_message = "Remote manager paused because WiFi is offline.";
  Serial.println(s_last_remote_message);
}

void ensure_server_state(void) {
  if (millis() - s_last_wifi_poll_ms < kWifiPollIntervalMs) {
    return;
  }
  s_last_wifi_poll_ms = millis();

  if (WiFi.status() == WL_CONNECTED) {
    start_remote_server();
    if (g_file_manager_ui.wifi_badge != nullptr) {
      lv_obj_clear_flag(g_file_manager_ui.wifi_badge, LV_OBJ_FLAG_HIDDEN);
    }
  } else {
    stop_remote_server();
    if (g_file_manager_ui.wifi_badge != nullptr) {
      lv_obj_add_flag(g_file_manager_ui.wifi_badge, LV_OBJ_FLAG_HIDDEN);
    }
  }

  if (g_file_manager_ui.status_label != nullptr) {
    update_status_label();
  }
}

void redirect_to_directory(const String &dir) {
  s_server.sendHeader("Location", "/?dir=" + url_encode(dir), true);
  s_server.send(303, "text/plain", "");
}

void handle_root_page(void) {
  const String dir_arg = s_server.hasArg("dir") ? url_decode(s_server.arg("dir")) : "/";
  const String dir_path = normalize_path(dir_arg, true);

  File dir = open_path_for_read(dir_path);
  if (!dir || !dir.isDirectory()) {
    if (dir) {
      dir.close();
    }
    s_server.send(404, "text/plain", "Directory not found.");
    return;
  }

  String html;
  html.reserve(4096);
  html += "<!doctype html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>ESP32 File Manager</title>";
  html += "<style>";
  html += "body{font-family:Segoe UI,Arial,sans-serif;background:#f4f7fb;color:#102a43;margin:0;padding:20px;}";
  html += ".card{max-width:960px;margin:0 auto;background:#fff;border-radius:18px;padding:20px;box-shadow:0 10px 28px rgba(16,42,67,.08);}";
  html += "h1{margin:0 0 8px;font-size:28px;} .muted{color:#486581;font-size:14px;margin-bottom:16px;}";
  html += ".row{display:flex;gap:12px;flex-wrap:wrap;margin-bottom:16px;} form{margin:0;} ";
  html += "input,button{font:inherit;padding:10px 12px;border-radius:10px;border:1px solid #cbd5e1;background:#fff;} ";
  html += "button{background:#2793e6;border:none;color:#fff;cursor:pointer;} ";
  html += "table{width:100%;border-collapse:collapse;margin-top:10px;} th,td{padding:12px 10px;border-bottom:1px solid #e6edf5;text-align:left;font-size:14px;} ";
  html += "a{color:#1769aa;text-decoration:none;} .danger{background:#d64545;} .pill{display:inline-block;padding:4px 8px;border-radius:999px;background:#e8f1fb;color:#1769aa;font-size:12px;}";
  html += "</style></head><body><div class='card'>";
  html += "<h1>ESP32 SD File Manager</h1>";
  html += "<div class='muted'>Current path: ";
  html += html_escape(dir_path);
  html += " | Device IP: ";
  html += html_escape(WiFi.localIP().toString());
  html += "</div>";

  html += "<div class='row'>";
  if (dir_path != "/") {
    html += "<a class='pill' href='/?dir=" + url_encode(parent_path(dir_path)) + "'>Up One Level</a>";
  }
  html += "<a class='pill' href='/?dir=%2F'>Root</a>";
  html += "</div>";

  html += "<div class='row'>";
  html += "<form action='/mkdir' method='get'>";
  html += "<input type='hidden' name='dir' value='" + html_escape(dir_path) + "'>";
  html += "<input type='text' name='name' placeholder='New folder name'>";
  html += "<button type='submit'>Create Folder</button></form>";

  // Put the directory in the query: POST fields are unavailable at UPLOAD_FILE_START.
  html += "<form id='upload-form' action='/upload?dir=" + url_encode(dir_path) + "' method='post' enctype='multipart/form-data'>";
  html += "<input id='upload-file' type='file' name='upload' required>";
  html += "<button type='submit'>Upload File</button></form>";
  html += "<div id='transfer-status' role='status' aria-live='polite' hidden>";
  html += "<progress id='transfer-progress' max='100' value='0'></progress> <span id='transfer-text'></span></div>";
  html += "</div>";

  html += "<table><thead><tr><th>Name</th><th>Type</th><th>Size</th><th>Action</th></tr></thead><tbody>";

  File file = dir.openNextFile();
  while (file) {
    // As in refresh_file_list(): re-build the full path from the directory we
    // asked for plus the basename — file.name() cannot be trusted to include it.
    const String raw_name = String(file.name());
    const String name = basename_from_path(normalize_path(raw_name, false));
    const String full_path = join_path(dir_path, name);
    html += "<tr><td>";
    if (file.isDirectory()) {
      html += "<a href='/?dir=" + url_encode(full_path) + "'>" + html_escape(name) + "</a>";
    } else {
      html += html_escape(name);
    }
    html += "</td><td>";
    html += file.isDirectory() ? "Folder" : "File";
    html += "</td><td>";
    if (file.isDirectory()) {
      html += "--";
    } else {
      char size_text[24];
      format_size(static_cast<size_t>(file.size()), size_text, sizeof(size_text));
      html += size_text;
    }
    html += "</td><td>";
    if (!file.isDirectory()) {
      html += "<a data-download='1' href='/download?path=" + url_encode(full_path) + "'>Download</a> ";
    }
    html += "<a href='/delete?path=" + url_encode(full_path) + "&dir=" + url_encode(dir_path) + "'>Delete</a>";
    html += "</td></tr>";

    file.close();
    file = dir.openNextFile();
  }

  html += R"HTML(</tbody></table></div>
<script>
(() => {
  const form = document.getElementById('upload-form');
  const input = document.getElementById('upload-file');
  const status = document.getElementById('transfer-status');
  const progress = document.getElementById('transfer-progress');
  const label = document.getElementById('transfer-text');
  let busy = false;
  function lock(value) {
    busy = value;
    document.querySelectorAll('input,button').forEach(el => el.disabled = value);
  }
  document.addEventListener('click', event => {
    if (busy && event.target.closest('a')) {
      event.preventDefault();
      return;
    }
    const link = event.target.closest('a[data-download]');
    if (!link) return;
    event.preventDefault();
    const filename = new URL(link.href).searchParams.get('path').split('/').pop();
    const xhr = new XMLHttpRequest();
    function fail(message) {
      lock(false);
      label.textContent = message;
    }
    xhr.open('GET', link.href);
    xhr.responseType = 'blob';
    xhr.onprogress = event => {
      if (!event.lengthComputable || event.total <= 0) return;
      const percent = Math.min(99, Math.floor(event.loaded * 100 / event.total));
      progress.value = percent;
      label.textContent = 'Downloading ' + filename + ': ' + percent + '%';
    };
    xhr.onload = async () => {
      if (xhr.status !== 200) { fail(await xhr.response.text()); return; }
      const url = URL.createObjectURL(xhr.response);
      const save = document.createElement('a');
      save.href = url;
      save.download = filename;
      document.body.appendChild(save);
      lock(false);
      save.click();
      save.remove();
      setTimeout(() => URL.revokeObjectURL(url), 60000);
      progress.value = 100;
      label.textContent = 'Download complete: 100%';
    };
    xhr.onerror = () => fail('Download interrupted. Check WiFi and retry.');
    xhr.onabort = () => fail('Download cancelled.');
    status.hidden = false;
    progress.value = 0;
    label.textContent = 'Downloading ' + filename + ': 0%';
    lock(true);
    xhr.send();
  });
  window.addEventListener('beforeunload', event => {
    if (busy) { event.preventDefault(); event.returnValue = ''; }
  });
  form.addEventListener('submit', event => {
    event.preventDefault();
    if (busy || !input.files.length) return;
    const file = input.files[0];
    const data = new FormData();
    data.append('upload', file, file.name);
    const url = new URL(form.action);
    url.searchParams.set('size', file.size);
    url.searchParams.set('ajax', '1');
    const xhr = new XMLHttpRequest();
    let finished = false;
    function fail(message) {
      if (finished) return;
      finished = true;
      lock(false);
      label.textContent = message;
    }
    xhr.open('POST', url);
    xhr.upload.onprogress = event => {
      if (!event.lengthComputable || event.total <= 0) return;
      const percent = Math.min(99, Math.floor(event.loaded * 100 / event.total));
      progress.value = percent;
      label.textContent = 'Uploading ' + file.name + ': ' + percent + '%';
    };
    xhr.upload.onload = () => label.textContent = 'Saving file on device...';
    xhr.onload = () => {
      if (xhr.status !== 200) { fail(xhr.responseText || 'Upload failed.'); return; }
      finished = true;
      progress.value = 100;
      label.textContent = 'Upload complete: 100%';
      lock(false);
      setTimeout(() => window.location.reload(), 600);
    };
    xhr.onerror = () => fail('Connection lost. Check WiFi and retry.');
    xhr.onabort = () => fail('Upload cancelled.');
    status.hidden = false;
    progress.value = 0;
    label.textContent = 'Uploading ' + file.name + ': 0%';
    lock(true);
    xhr.send(data);
  });
})();
</script></body></html>)HTML";
  dir.close();

  s_server.send(200, "text/html", html);
}

void handle_download(void) {
  if (!s_server.hasArg("path")) {
    s_server.send(400, "text/plain", "Missing path.");
    return;
  }

  const String path = normalize_path(url_decode(s_server.arg("path")), false);
  File file = open_path_for_read(path);
  if (!file || file.isDirectory()) {
    if (file) {
      file.close();
    }
    s_server.send(404, "text/plain", "File not found.");
    return;
  }

  const size_t total = file.size();
  if (!all_in_one_file_transfer_begin(basename_from_path(path).c_str(), false)) {
    file.close();
    s_server.send(503, "text/plain", "Device is busy. Please retry.");
    return;
  }
  s_server.sendHeader(
    "Content-Disposition",
    "attachment; filename=\"" + basename_from_path(path) + "\"");
  s_server.setContentLength(total);
  s_server.send(200, "application/octet-stream", "");
  auto client = s_server.client();
  size_t sent = 0;
  uint8_t buffer[1024];
  all_in_one_file_transfer_update(0, total);
  while (sent < total && client.connected() && WiFi.status() == WL_CONNECTED) {
    const size_t count = file.read(buffer, min(sizeof(buffer), total - sent));
    if (count == 0) {
      break;
    }
    size_t offset = 0;
    while (offset < count && client.connected()) {
      const size_t written = client.write(buffer + offset, count - offset);
      if (written == 0) {
        break;
      }
      offset += written;
      sent += written;
      all_in_one_file_transfer_update(sent, total);
      delay(1);
    }
    if (offset != count) {
      break;
    }
  }
  file.close();
  if (sent == total) {
    all_in_one_file_transfer_update(sent, total, true);
    s_last_remote_message = "Download finished.";
  } else {
    s_last_remote_message = "Download interrupted.";
    client.stop();
  }
  all_in_one_file_transfer_end();
}

void handle_delete(void) {
  if (!s_server.hasArg("path")) {
    s_server.send(400, "text/plain", "Missing path.");
    return;
  }

  const String path = normalize_path(url_decode(s_server.arg("path")), false);
  const String dir = s_server.hasArg("dir") ? normalize_path(url_decode(s_server.arg("dir")), true) : "/";

  bool deleted = false;
  File entry = open_path_for_read(path);
  if (entry) {
    const bool is_dir = entry.isDirectory();
    entry.close();
    deleted = is_dir ? remove_dir_path(path) : remove_file_path(path);
  }

  s_last_remote_message = deleted ? String("Deleted remotely: ") + path : String("Delete failed for ") + path;
  s_file_list_dirty = deleted;
  redirect_to_directory(dir);
}

void handle_mkdir(void) {
  const String dir = s_server.hasArg("dir") ? normalize_path(url_decode(s_server.arg("dir")), true) : "/";
  const String name = s_server.hasArg("name") ? url_decode(s_server.arg("name")) : "";
  if (name.length() == 0 || name.indexOf('/') >= 0 || name.indexOf("..") >= 0) {
    s_server.send(400, "text/plain", "Invalid folder name.");
    return;
  }

  const String new_dir = join_path(dir, name);
  bool ok = SD_MMC.mkdir(new_dir.c_str());
  if (!ok) {
    ok = SD_MMC.mkdir(mounted_path(new_dir).c_str());
  }
  s_last_remote_message = ok ? String("Created folder remotely: ") + new_dir : String("Create folder failed: ") + new_dir;
  s_file_list_dirty = ok;
  redirect_to_directory(dir);
}

// These handlers run synchronously inside handleClient() on the LVGL main task.
// A temporary file prevents interrupted uploads from destroying an existing file.
void finish_upload(bool success, const char *message) {
  if (s_upload_file) {
    s_upload_file.close();
  }
  if (!success && s_upload_temp_path.length() > 0) {
    remove_file_path(s_upload_temp_path);
  }
  s_upload_ok = success;
  s_upload_result_ready = true;
  s_upload_in_progress = false;
  s_last_remote_message = message;
  if (!success) {
    s_upload_error = message;
  } else {
    s_file_list_dirty = true;
    all_in_one_file_transfer_update(s_upload_written, s_upload_written, true);
  }
  if (s_upload_owns_pause) {
    all_in_one_file_transfer_end();
    s_upload_owns_pause = false;
  }
}

bool commit_upload(void) {
  const String backup = s_upload_temp_path + ".bak";
  const bool replacing = path_exists(s_upload_path);
  if (replacing && !SD_MMC.rename(s_upload_path.c_str(), backup.c_str())) {
    return false;
  }
  if (!SD_MMC.rename(s_upload_temp_path.c_str(), s_upload_path.c_str())) {
    if (replacing) {
      SD_MMC.rename(backup.c_str(), s_upload_path.c_str());
    }
    return false;
  }
  if (replacing) {
    remove_file_path(backup);
  }
  return true;
}

void handle_upload_finish(void) {
  if (!s_upload_result_ready) {
    s_server.send(400, "text/plain", "No file was uploaded.");
  } else if (!s_upload_ok) {
    s_server.send(500, "text/plain", s_upload_error);
  } else if (s_server.hasArg("ajax")) {
    s_server.send(200, "text/plain", "Upload complete.");
  } else {
    redirect_to_directory(s_upload_directory);
  }
  s_upload_result_ready = false;
}

void handle_upload_stream(void) {
  HTTPUpload &upload = s_server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    s_upload_ok = false;
    s_upload_result_ready = false;
    s_upload_in_progress = true;
    s_upload_written = 0;
    s_upload_error = "";
    s_upload_temp_path = "";
    // WebServer already URL-decodes query arguments.
    s_upload_directory = s_server.hasArg("dir") ? normalize_path(s_server.arg("dir"), true) : "/";
    String filename = upload.filename;
    const int slash = filename.lastIndexOf('/');
    const int backslash = filename.lastIndexOf('\\');
    const int separator = slash > backslash ? slash : backslash;
    if (separator >= 0) {
      filename = filename.substring(separator + 1);
    }
    s_upload_path = join_path(s_upload_directory, filename);
    s_upload_expected = 0;
    s_upload_has_expected = s_server.hasArg("size");
    if (s_upload_has_expected) {
      const String size_arg = s_server.arg("size");
      char *end = nullptr;
      const unsigned long long expected = strtoull(size_arg.c_str(), &end, 10);
      if (size_arg.isEmpty() || *end != '\0' || expected > UINT32_MAX) {
        s_upload_error = "Invalid upload size.";
      } else {
        s_upload_expected = static_cast<size_t>(expected);
      }
    }
    if (filename.isEmpty() || filename.indexOf("..") >= 0 || filename == ".") {
      s_upload_error = "Invalid filename.";
    }
    File directory = open_path_for_read(s_upload_directory);
    if (!directory || !directory.isDirectory()) {
      s_upload_error = "Upload directory not found.";
    }
    directory.close();
    File existing = open_path_for_read(s_upload_path);
    if (existing && existing.isDirectory()) {
      s_upload_error = "A folder already has that name.";
    }
    existing.close();
    if (s_upload_error.isEmpty()) {
      s_upload_owns_pause = all_in_one_file_transfer_begin(filename.c_str(), true);
      if (!s_upload_owns_pause) {
        s_upload_error = "Device is busy. Please retry.";
      }
    }
    if (s_upload_error.isEmpty()) {
      unsigned counter = 0;
      do {
        s_upload_temp_path = join_path(s_upload_directory,
          String(".upload-") + String(millis()) + "-" + String(counter++) + ".tmp");
      } while (path_exists(s_upload_temp_path) || path_exists(s_upload_temp_path + ".bak"));
      s_upload_file = open_path_for_write(s_upload_temp_path);
      if (!s_upload_file) {
        s_upload_error = "Unable to create file on SD card.";
      }
    }
    all_in_one_file_transfer_update(0, s_upload_expected);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (s_upload_in_progress && s_upload_error.isEmpty() && s_upload_file) {
      const size_t written = s_upload_file.write(upload.buf, upload.currentSize);
      s_upload_written += written;
      if (written != upload.currentSize) {
        s_upload_error = "SD card write failed (card full or unavailable).";
      } else if (s_upload_has_expected && s_upload_written > s_upload_expected) {
        s_upload_error = "Upload size mismatch.";
      }
      all_in_one_file_transfer_update(s_upload_written, s_upload_expected);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (!s_upload_in_progress) {
      return;
    }
    if (s_upload_file) {
      s_upload_file.close();
    }
    if (s_upload_error.isEmpty() &&
        (s_upload_written != upload.totalSize ||
         (s_upload_has_expected && s_upload_written != s_upload_expected))) {
      s_upload_error = "Upload incomplete: size mismatch.";
    }
    if (s_upload_error.isEmpty() && !commit_upload()) {
      s_upload_error = "Unable to save uploaded file.";
    }
    if (s_upload_error.isEmpty()) {
      finish_upload(true, "Upload finished.");
    } else {
      const String error = s_upload_error;
      finish_upload(false, error.c_str());
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED && s_upload_in_progress) {
    finish_upload(false, "Upload interrupted. Please retry.");
  }
}

void handle_not_found(void) {
  s_server.send(404, "text/plain", "Not found.");
}

void exit_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  if (strcmp(s_current_path, "/") == 0) {
    all_in_one_show_home();
  } else {
    navigate_to_parent();
  }
}

void refresh_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  refresh_file_list();
}

void file_item_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  const uint8_t entry_index = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
  if (entry_index == kParentEntryIndex) {
    navigate_to_parent();
    return;
  }

  if (entry_index >= s_entry_count) {
    return;
  }

  if (s_entries[entry_index].is_dir) {
    navigate_to(s_entries[entry_index].path);
    return;
  }

  show_file_dialog(entry_index);
}

void cancel_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  hide_dialog();
}

void delete_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  if (s_selected_file_path[0] == '\0') {
    hide_dialog();
    return;
  }

  if (remove_file_path(String(s_selected_file_path))) {
    s_last_remote_message = String("Deleted locally: ") + s_selected_file_name;
  } else {
    s_last_remote_message = String("Delete failed: ") + s_selected_file_name;
  }

  hide_dialog();
  refresh_file_list();
}

}  // namespace

FileManagerUI g_file_manager_ui = {};

void file_manager_ui_setup(FileManagerUI *ui) {
  create_styles();

  ui->screen = lv_obj_create(nullptr);
  lv_obj_add_style(ui->screen, &s_screen_style, LV_PART_MAIN);
  lv_obj_clear_flag(ui->screen, LV_OBJ_FLAG_SCROLLABLE);

  build_header(ui);
  build_file_list(ui);
  build_footer(ui);
  build_dialog(ui);

  copy_string(s_current_path, sizeof(s_current_path), String("/"));
  refresh_file_list();
}

void file_manager_ui_loop(void) {
  ensure_server_state();
}

void file_manager_ui_background_loop(void) {
  ensure_server_state();
  if (s_server_started) {
    s_server.handleClient();
    // Parsing can return early without END/ABORT after a malformed request.
    if (s_upload_in_progress) {
      finish_upload(false, "Upload interrupted. Please retry.");
    }
    s_upload_result_ready = false;
  }
  if (s_file_list_dirty) {
    hide_dialog();
    refresh_file_list();
    s_file_list_dirty = false;
  }
}

void file_manager_ui_refresh(void) {
  hide_dialog();
  refresh_file_list();
}
