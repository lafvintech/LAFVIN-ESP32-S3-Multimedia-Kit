#include "File_Manager_ui.h"

#include "FS.h"
#include "SD_MMC.h"
#include "img/img_index.h"

#include <cstdio>
#include <cstring>

namespace {

constexpr uint8_t kMaxEntries = 48;
constexpr size_t kMaxPathLength = 192;
constexpr size_t kMaxNameLength = 80;
constexpr uint8_t kParentEntryIndex = 255;

constexpr int kSdCmdPin = 38;
constexpr int kSdClkPin = 39;
constexpr int kSdD0Pin = 40;

struct FileEntry {
  char name[kMaxNameLength];
  char path[kMaxPathLength];
  bool is_dir;
  size_t size_bytes;
};

FileEntry s_entries[kMaxEntries] = {};
uint8_t s_entry_count = 0;
bool s_sd_ready = false;

char s_current_path[kMaxPathLength] = "/";
char s_selected_file_path[kMaxPathLength] = {};
char s_selected_file_name[kMaxNameLength] = {};

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

static bool init_sd_card(void);
static void refresh_file_list(void);
static void update_path_label(void);
static void set_status_text(const char *text);
static void hide_dialog(void);
static void show_file_dialog(uint8_t entry_index);
static bool navigate_to(const char *path);
static bool navigate_to_parent(void);

static String basename_from_path(const String &full_path);
static void copy_string(char *dest, size_t dest_size, const String &value);
static String normalize_path(const String &raw_path, bool directory_path);
static void format_size(size_t size_bytes, char *buffer, size_t buffer_size);
static void update_delete_button_state(bool enabled);

static void exit_event_handler(lv_event_t *e);
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
  lv_img_set_src(exit_img, GALLERY_IMG_ESC);
  lv_obj_center(exit_img);

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
  lv_obj_set_pos(ui->title_label, 66, 10);
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
  lv_obj_set_size(ui->list, 300, 146);
  lv_obj_add_style(ui->list, &s_list_style, LV_PART_MAIN);
}

void build_footer(FileManagerUI *ui) {
  ui->status_label = lv_label_create(ui->screen);
  lv_obj_set_pos(ui->status_label, 12, 214);
  lv_obj_set_width(ui->status_label, 296);
  lv_label_set_long_mode(ui->status_label, LV_LABEL_LONG_WRAP);
  lv_label_set_text(ui->status_label, "Waiting for SD card...");
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

bool init_sd_card(void) {
  if (s_sd_ready) {
    return true;
  }

  Serial.println("Initializing SD card for file manager...");
  SD_MMC.setPins(kSdClkPin, kSdCmdPin, kSdD0Pin);

  s_sd_ready = SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5);
  if (!s_sd_ready) {
    Serial.println("SD card mount failed.");
  } else {
    Serial.println("SD card ready.");
  }
  return s_sd_ready;
}

void update_path_label(void) {
  lv_label_set_text_fmt(g_file_manager_ui.path_label, "Path: %s", s_current_path);
}

void set_status_text(const char *text) {
  lv_label_set_text(g_file_manager_ui.status_label, text);
}

void hide_dialog(void) {
  lv_obj_add_flag(g_file_manager_ui.overlay, LV_OBJ_FLAG_HIDDEN);
  s_selected_file_path[0] = '\0';
  s_selected_file_name[0] = '\0';
}

bool navigate_to(const char *path) {
  if (path == nullptr || path[0] == '\0') {
    return false;
  }

  const String normalized = normalize_path(path, true);
  File dir = SD_MMC.open(normalized.c_str());
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
  if (strcmp(s_current_path, "/") == 0) {
    return false;
  }

  String path = normalize_path(String(s_current_path), true);
  const int last_slash = path.lastIndexOf('/');
  if (last_slash <= 0) {
    return navigate_to("/");
  }

  path.remove(last_slash);
  return navigate_to(path.c_str());
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

String normalize_path(const String &raw_path, bool directory_path) {
  String path = raw_path;
  path.replace('\\', '/');

  if (path.startsWith("/sdcard")) {
    path.remove(0, 7);
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
  if (directory_path && path.length() > 1 && path.endsWith("/")) {
    path.remove(path.length() - 1);
  }
  return path;
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

void refresh_file_list(void) {
  update_path_label();
  lv_obj_clean(g_file_manager_ui.list);

  if (!init_sd_card()) {
    set_status_text("SD card mount failed. Check the card and refresh again.");
    lv_obj_t *label = lv_label_create(g_file_manager_ui.list);
    lv_label_set_text(label, "SD card is not available.");
    return;
  }

  File dir = SD_MMC.open(s_current_path);
  if (!dir || !dir.isDirectory()) {
    if (dir) {
      dir.close();
    }
    set_status_text("Current path is unavailable. Returning to root.");
    copy_string(s_current_path, sizeof(s_current_path), String("/"));
    update_path_label();
    dir = SD_MMC.open(s_current_path);
    if (!dir || !dir.isDirectory()) {
      if (dir) {
        dir.close();
      }
      set_status_text("Unable to open the SD card root directory.");
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
    // file.name() may return a bare basename on newer ESP32 Arduino cores, so
    // always rebuild the full path from the current directory to ensure that
    // the stored path points at the real entry for remove() / open().
    const String raw_name = String(file.name());
    const String entry_name = basename_from_path(normalize_path(raw_name, false));
    String full_path;
    if (strcmp(s_current_path, "/") == 0) {
      full_path = normalize_path("/" + entry_name, file.isDirectory());
    } else {
      full_path = normalize_path(String(s_current_path) + "/" + entry_name, file.isDirectory());
    }
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

  char status_buffer[96];
  const uint64_t total_mb = SD_MMC.totalBytes() / (1024ULL * 1024ULL);
  const uint64_t used_mb = SD_MMC.usedBytes() / (1024ULL * 1024ULL);
  snprintf(
    status_buffer,
    sizeof(status_buffer),
    "Items: %u  |  Storage: %llu / %llu MB",
    static_cast<unsigned>(s_entry_count),
    used_mb,
    total_mb);
  set_status_text(status_buffer);
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

void exit_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  navigate_to_parent();
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

  if (SD_MMC.remove(s_selected_file_path)) {
    char status_buffer[96];
    snprintf(status_buffer, sizeof(status_buffer), "Deleted: %s", s_selected_file_name);
    set_status_text(status_buffer);
  } else {
    char status_buffer[96];
    snprintf(status_buffer, sizeof(status_buffer), "Delete failed: %s", s_selected_file_name);
    set_status_text(status_buffer);
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

  refresh_file_list();
}

void file_manager_ui_loop(void) {
}
