#include "all_in_one_app.h"

#include "home_ui.h"

#include "Buzzer_ui.h"
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

static bool s_sd_attempted = false;
static bool s_sd_ready = false;
static bool s_camera_init_attempted = false;
static bool s_camera_initialized = false;

static bool s_ntp_requested = false;
static bool s_ntp_synced = false;
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
  home_ui_set_time_text("--:--");
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
    home_ui_set_time_text("--:--");
    return;
  }

  time_t now = time(nullptr);
  if (now < 100000) {
    home_ui_set_time_text("--:--");
    return;
  }

  s_ntp_synced = true;

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
  all_in_one_show_home();
}

void all_in_one_app_loop(void) {
  // Keep time fresh even while the user is on the home screen.
  refresh_clock_label();

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
    default:
      break;
  }
}

void all_in_one_show_home(void) {
  ensure_home_ready();

  // Cleanly leave the previous module before returning home.
  before_switch_from(s_current_screen);

  lv_scr_load(g_home_ui.screen);
  home_ui_show_page(s_home_page, false);
  s_current_screen = APP_SCREEN_HOME;
}

void all_in_one_open_screen(AppScreen screen) {
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
  if (s_ntp_requested) {
    return;
  }

  // Use China Standard Time and start NTP once WiFi is confirmed.
  setenv("TZ", "CST-8", 1);
  tzset();
  configTime(8 * 3600, 0, "ntp.aliyun.com", "pool.ntp.org");
  s_ntp_requested = true;
  s_ntp_synced = false;
  s_last_clock_refresh_ms = 0;
}
