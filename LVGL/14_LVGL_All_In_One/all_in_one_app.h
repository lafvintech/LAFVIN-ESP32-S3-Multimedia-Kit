#ifndef __ALL_IN_ONE_APP_H
#define __ALL_IN_ONE_APP_H

#include "Arduino.h"
#include "lvgl.h"

// Screens managed by the shared all-in-one shell.
enum AppScreen {
  APP_SCREEN_HOME = 0,
  APP_SCREEN_CAMERA,
  APP_SCREEN_GALLERY,
  APP_SCREEN_MUSIC,
  APP_SCREEN_HEARTRATE,
  APP_SCREEN_RGB,
  APP_SCREEN_BUZZER,
  APP_SCREEN_WIFI,
};

// Initialize the app shell and load the home screen.
void all_in_one_app_init(void);

// Run the background logic for the currently active module.
void all_in_one_app_loop(void);

// Return to the home screen while preserving the current home page.
void all_in_one_show_home(void);

// Open one of the integrated demo screens.
void all_in_one_open_screen(AppScreen screen);

// Accessors for the current home tile page.
uint8_t all_in_one_get_home_page(void);
void all_in_one_set_home_page(uint8_t page_index);

// Notify the shell that WiFi is ready so NTP can start syncing time.
void all_in_one_notify_wifi_connected(void);

#endif
