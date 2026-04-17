#ifndef __BUZZER_UI_H
#define __BUZZER_UI_H

#include "Arduino.h"
#include "lvgl.h"

#define BUZZER_PIN 45
#define PIANO_KEY_COUNT 7
#define BLACK_KEY_COUNT 5

typedef struct {
  lv_obj_t *screen;
  lv_obj_t *exit_button;
  lv_obj_t *exit_label;

  lv_obj_t *piano_panel;
  lv_obj_t *key_buttons[PIANO_KEY_COUNT];
  lv_obj_t *black_keys[BLACK_KEY_COUNT];
  lv_obj_t *volume_slider;
  lv_obj_t *power_switch;
  lv_obj_t *power_label;
} BuzzerUI;

extern BuzzerUI g_buzzer_ui;

void buzzer_ui_setup(BuzzerUI *ui);
void buzzer_ui_loop(void);

#endif
