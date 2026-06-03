#ifndef __RGB_UI_H
#define __RGB_UI_H

#include "Arduino.h"
#include "lvgl.h"

// Built-in WS2812 on this board.
#define RGB_LED_PIN 48

// LVGL objects used by the RGB control page.
typedef struct {
  lv_obj_t *screen;
  lv_obj_t *exit_button;
  lv_obj_t *exit_label;

  lv_obj_t *title_label;
  lv_obj_t *subtitle_label;

  lv_obj_t *preview_panel;
  lv_obj_t *preview_light;
  lv_obj_t *preview_name;

  lv_obj_t *brightness_slider;
  lv_obj_t *brightness_value;

  lv_obj_t *preset_buttons[6];
  lv_obj_t *mode_buttons[4];
} RGBUI;

extern RGBUI g_rgb_ui;

// Build the RGB screen and initialize the first LED state.
void rgb_ui_setup(RGBUI *ui);

// Update animations and refresh visible state.
void rgb_ui_loop(void);

#endif
