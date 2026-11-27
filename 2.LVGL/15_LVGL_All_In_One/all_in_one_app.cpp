#include "all_in_one_app.h"

#include "home_ui.h"

#include "Buzzer_ui.h"
#include "File_Manager_ui.h"
#include "RGB_ui.h"
#include "Wifi_ui.h"
#include "camera.h"
#include "camera_ui.h"
#include "hartrate_ui.h"
#include "music_ui.h"
#include "picture_ui.h"
#include "sd_card.h"

#include "WiFi.h"

#include <time.h>

namespace {

// Global shell state shared by all integrated demo modules.
static AppScreen s_current_screen = APP_SCREEN_HOME;
static uint8_t s_home_page = 0;

// Lazy initialization flags keep heavy modules from starting until needed.
static bool s_home_ready = false;
static bool s_camera_ready = false;
static bool s_gallery_ready = false;
static bool s_music_ready = false;
static bool s_heartrate_ready = false;
static bool s_rgb_ready = false;
static bool s_buzzer_ready = false;
static bool s_wifi_ready = false;
static bool s_file_manager_ready = false;

static bool s_sd_attempted = false;
static bool s_sd_ready = false;
static bool s_camera_init_attempted = false;
static bool s_camera_initialized = false;

static bool s_ntp_requested = false;
static bool s_wifi_connected = false;
static bool s_transfer_active = false;
static bool s_resume_camera = false;
static bool s_resume_music = false;
static lv_obj_t *s_transfer_overlay = nullptr;
static lv_obj_t *s_transfer_name = nullptr;
static lv_obj_t *s_transfer_progress = nullptr;
static lv_obj_t *s_transfer_bar = nullptr;
static uint32_t s_last_transfer_draw_ms = 0;
static uint32_t s_last_clock_refresh_ms = 0;

// Bring up the SD card once so dependent modules can reuse it.
static void ensure_sd_ready(void);

// Initialize the camera driver only on first use.
static void ensure_camera_ready(void);

// Build the home UI only once and reuse it.
static void ensure_home_ready(void);

// Refresh the home header clock once per second after NTP is enabled.
static void refresh_clock_label(void);

// Map an AppScreen enum to the actual LVGL screen object.
static lv_obj_t *screen_for(AppScreen screen);

// Stop background tasks before leaving a module screen.
static void before_switch_from(AppScreen screen);

void ensure_sd_ready(void) {
  if (s_sd_attempted) {
    return;
  }

  s_sd_attempted = true;
  s_sd_ready = sdcard_init() != 0;
}

void ensure_camera_ready(void) {
  if (s_camera_init_attempted) {
    return;
  }

  s_camera_init_attempted = true;
  s_camera_initialized = camera_init();
}

void ensure_home_ready(void) {
  if (s_home_ready) {
    return;
  }

  home_ui_setup(&g_home_ui);
  home_ui_set_time_text("12:34");
  s_home_ready = true;
}

lv_obj_t *screen_for(AppScreen screen) {
  // Some legacy modules expose a different screen symbol, so centralize it here.
  switch (screen) {
    case APP_SCREEN_HOME:
      return g_home_ui.screen;
    case APP_SCREEN_CAMERA:
      return g_camera_ui.screen;
    case APP_SCREEN_GALLERY:
      return guider_picture_ui.picture;
    case APP_SCREEN_MUSIC:
      return g_music_ui.screen;
    case APP_SCREEN_HEARTRATE:
      return g_heartrate_ui.screen;
    case APP_SCREEN_RGB:
      return g_rgb_ui.screen;
    case APP_SCREEN_BUZZER:
      return g_buzzer_ui.screen;
    case APP_SCREEN_WIFI:
      return g_wifi_ui.screen;
    case APP_SCREEN_FILE_MANAGER:
      return g_file_manager_ui.screen;
    default:
      return nullptr;
  }
}

void before_switch_from(AppScreen screen) {
  // Stop hardware or timing loops that should not keep running in the background.
  if (screen == APP_SCREEN_CAMERA && camera_task_is_running()) {
    camera_task_stop();
  }
  if (screen == APP_SCREEN_HEARTRATE) {
    hartrate_ui_stop();
  }
  if (screen == APP_SCREEN_MUSIC) {
    music_ui_stop();
  }
  if (screen == APP_SCREEN_RGB) {
    rgb_ui_stop();
  }
}

void refresh_clock_label(void) {
  // Update the clock at a steady 1 Hz pace to avoid unnecessary redraws.
  if (millis() - s_last_clock_refresh_ms < 1000) {
    return;
  }
  s_last_clock_refresh_ms = millis();

  if (!s_ntp_requested) {
    home_ui_set_time_text("12:34");
    return;
  }

  time_t now = time(nullptr);
  if (now < 1700000000) {
    home_ui_set_time_text("12:34");
    return;
  }

  struct tm time_info;
  localtime_r(&now, &time_info);
  char buffer[6];
  strftime(buffer, sizeof(buffer), "%H:%M", &time_info);
  home_ui_set_time_text(buffer);
}

}  // namespace

void all_in_one_app_init(void) {
  // The shell starts with storage and the home screen ready.
  ensure_sd_ready();
  ensure_home_ready();
  home_ui_set_wifi_visible(WiFi.status() == WL_CONNECTED);
  all_in_one_show_home();
}

void all_in_one_app_loop(void) {
  // Observe connections globally, including connections completed off the WiFi page.
  const bool connected = WiFi.status() == WL_CONNECTED;
  if (connected && !s_wifi_connected) {
    all_in_one_notify_wifi_connected();
  }
  s_wifi_connected = connected;
  refresh_clock_label();
  home_ui_set_wifi_visible(connected);
  if (!s_transfer_active && s_resume_camera && !camera_task_is_running()) {
    s_resume_camera = false;
    camera_task_start();
  }
  if (s_file_manager_ready) {
    file_manager_ui_background_loop();
  }

  // Dispatch the per-frame loop only to the currently active module.
  switch (s_current_screen) {
    case APP_SCREEN_CAMERA:
      camera_ui_refresh_preview();
      break;
    case APP_SCREEN_MUSIC:
      music_ui_loop();
      break;
    case APP_SCREEN_HEARTRATE:
      hartrate_ui_loop();
      break;
    case APP_SCREEN_RGB:
      rgb_ui_loop();
      break;
    case APP_SCREEN_BUZZER:
      buzzer_ui_loop();
      break;
    case APP_SCREEN_WIFI:
      wifi_ui_loop();
      break;
    case APP_SCREEN_FILE_MANAGER:
      file_manager_ui_loop();
      break;
    default:
      break;
  }
}

void all_in_one_show_home(void) {
  if (s_transfer_active) {
    return;
  }
  ensure_home_ready();

  // Cleanly leave the previous module before returning home.
  before_switch_from(s_current_screen);

  lv_scr_load(g_home_ui.screen);
  home_ui_show_page(s_home_page, false);
  s_current_screen = APP_SCREEN_HOME;
}

void all_in_one_open_screen(AppScreen screen) {
  if (s_transfer_active) {
    return;
  }
  ensure_home_ready();

  // Treat a request for HOME as a normal back-to-home action.
  if (screen == APP_SCREEN_HOME) {
    all_in_one_show_home();
    return;
  }

  // Stop the currently active module before opening another one.
  before_switch_from(s_current_screen);

  switch (screen) {
    case APP_SCREEN_CAMERA:
      ensure_sd_ready();
      ensure_camera_ready();
      if (!s_camera_initialized) {
        all_in_one_show_home();
        return;
      }
      if (!s_camera_ready) {
        camera_ui_setup(&g_camera_ui);
        s_camera_ready = true;
      }
      camera_task_start();
      break;

    case APP_SCREEN_GALLERY:
      ensure_sd_ready();
      if (!s_gallery_ready) {
        setup_scr_picture(&guider_picture_ui);
        s_gallery_ready = true;
      }
      picture_ui_refresh();
      break;

    case APP_SCREEN_MUSIC:
      ensure_sd_ready();
      if (!s_music_ready) {
        music_ui_setup(&g_music_ui);
        s_music_ready = true;
      }
      break;

    case APP_SCREEN_HEARTRATE:
      if (!s_heartrate_ready) {
        hartrate_ui_setup(&g_heartrate_ui);
        s_heartrate_ready = true;
      } else {
        hartrate_ui_start();
      }
      break;

    case APP_SCREEN_RGB:
      if (!s_rgb_ready) {
        rgb_ui_setup(&g_rgb_ui);
        s_rgb_ready = true;
      }
      break;

    case APP_SCREEN_BUZZER:
      if (!s_buzzer_ready) {
        buzzer_ui_setup(&g_buzzer_ui);
        s_buzzer_ready = true;
      }
      break;

    case APP_SCREEN_WIFI:
      if (!s_wifi_ready) {
        wifi_ui_setup(&g_wifi_ui);
        s_wifi_ready = true;
      }
      break;

    case APP_SCREEN_FILE_MANAGER:
      ensure_sd_ready();
      if (!s_file_manager_ready) {
        file_manager_ui_setup(&g_file_manager_ui);
        s_file_manager_ready = true;
      }
      file_manager_ui_refresh();
      break;

    default:
      break;
  }

  // Only switch screens if the target module produced a valid LVGL root object.
  lv_obj_t *target_screen = screen_for(screen);
  if (target_screen != nullptr) {
    lv_scr_load(target_screen);
    s_current_screen = screen;
  }
}

uint8_t all_in_one_get_home_page(void) {
  return s_home_page;
}

void all_in_one_set_home_page(uint8_t page_index) {
  s_home_page = (page_index > 0) ? 1 : 0;
}

void all_in_one_notify_wifi_connected(void) {
  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  // Use China Standard Time and request fresh NTP time on each connection.
  setenv("TZ", "CST-8", 1);
  tzset();
  configTime(8 * 3600, 0, "ntp.aliyun.com", "pool.ntp.org");
  s_ntp_requested = true;
  s_last_clock_refresh_ms = millis() - 1000;
}


bool all_in_one_file_transfer_begin(const char *name, bool upload) {
  if (s_transfer_active) {
    return false;
  }
  s_transfer_active = true;
  lv_indev_reset(nullptr, nullptr);
  lv_indev_enable(nullptr, false);
  if (s_transfer_overlay == nullptr) {
    // The top layer covers whichever module is currently open.
    s_transfer_overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_transfer_overlay, 320, 240);
    lv_obj_set_pos(s_transfer_overlay, 0, 0);
    lv_obj_clear_flag(s_transfer_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_transfer_overlay, lv_color_hex(0x102A43), 0);
    lv_obj_set_style_bg_opa(s_transfer_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_transfer_overlay, 0, 0);
    lv_obj_set_style_radius(s_transfer_overlay, 0, 0);
    lv_obj_set_style_text_color(s_transfer_overlay, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_transfer_overlay, &lv_font_montserrat_16, 0);
    lv_obj_t *title = lv_label_create(s_transfer_overlay);
    lv_label_set_text(title, "Transferring...");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 22);
    s_transfer_name = lv_label_create(s_transfer_overlay);
    lv_obj_set_width(s_transfer_name, 280);
    lv_label_set_long_mode(s_transfer_name, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(s_transfer_name, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(s_transfer_name, LV_ALIGN_TOP_MID, 0, 58);
    s_transfer_bar = lv_bar_create(s_transfer_overlay);
    lv_obj_set_size(s_transfer_bar, 264, 18);
    lv_obj_align(s_transfer_bar, LV_ALIGN_TOP_MID, 0, 98);
    lv_bar_set_range(s_transfer_bar, 0, 100);
    s_transfer_progress = lv_label_create(s_transfer_overlay);
    lv_obj_align(s_transfer_progress, LV_ALIGN_TOP_MID, 0, 128);
    lv_obj_t *hint = lv_label_create(s_transfer_overlay);
    lv_label_set_text(hint, "Controls paused. Please wait.");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -12);
  }
  lv_label_set_text_fmt(s_transfer_name, "%s: %s", upload ? "Upload" : "Download", name);
  lv_obj_clear_flag(s_transfer_overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(s_transfer_overlay);
  lv_bar_set_value(s_transfer_bar, 0, LV_ANIM_OFF);
  lv_label_set_text(s_transfer_progress, "Preparing...");
  lv_refr_now(nullptr);

  s_resume_camera = s_current_screen == APP_SCREEN_CAMERA && camera_task_is_running();
  if (s_resume_camera && !camera_task_stop()) {
    all_in_one_file_transfer_end();
    return false;
  }
  s_resume_music = s_current_screen == APP_SCREEN_MUSIC && music_ui_pause_for_transfer();
  if (s_current_screen == APP_SCREEN_HEARTRATE) {
    hartrate_ui_stop();
  }
  if (s_current_screen == APP_SCREEN_BUZZER) {
    buzzer_ui_stop();
  }
  s_last_transfer_draw_ms = millis() - 100;
  return true;
}

void all_in_one_file_transfer_update(size_t done, size_t total, bool complete) {
  if (!s_transfer_active || (!complete && millis() - s_last_transfer_draw_ms < 100)) {
    return;
  }
  s_last_transfer_draw_ms = millis();
  if (total > 0 || complete) {
    const unsigned percent = complete ? 100 :
      static_cast<unsigned>(min(static_cast<uint64_t>(done) * 100 / total, uint64_t(99)));
    lv_bar_set_value(s_transfer_bar, percent, LV_ANIM_OFF);
    lv_label_set_text_fmt(s_transfer_progress, "%u%%  (%lu / %lu KB)", percent,
                          static_cast<unsigned long>(done / 1024),
                          static_cast<unsigned long>(total / 1024));
  } else {
    lv_label_set_text_fmt(s_transfer_progress, "%lu KB received",
                          static_cast<unsigned long>(done / 1024));
  }
  // Draw only. Running LVGL timers here would allow reentrant input callbacks.
  lv_refr_now(nullptr);
}

void all_in_one_file_transfer_end(void) {
  if (!s_transfer_active) {
    return;
  }
  if (s_transfer_overlay != nullptr) {
    lv_obj_add_flag(s_transfer_overlay, LV_OBJ_FLAG_HIDDEN);
  }
  s_transfer_active = false;
  lv_indev_reset(nullptr, nullptr);
  lv_indev_enable(nullptr, true);
  for (lv_indev_t *input = lv_indev_get_next(nullptr); input != nullptr;
       input = lv_indev_get_next(input)) {
    lv_indev_wait_release(input);
  }
  if (s_resume_music) {
    music_ui_resume_after_transfer();
    s_resume_music = false;
  }
  if (s_current_screen == APP_SCREEN_HEARTRATE) {
    hartrate_ui_start();
  }
  // Camera restart is deferred to the next main-loop tick, after HTTP returns.
  lv_refr_now(nullptr);
}
