#ifndef __MUSIC_UI_H
#define __MUSIC_UI_H

#include "Arduino.h"
#include "lvgl.h"

#define I2S_BCLK 42
#define I2S_DOUT 41
#define I2S_LRC  14

// Main UI object set for the music demo screen.
typedef struct {
  lv_obj_t *screen;
  lv_obj_t *exit_button;
  lv_obj_t *prev_button;
  lv_obj_t *play_pause_button;
  lv_obj_t *stop_button;
  lv_obj_t *next_button;
  lv_obj_t *title_label;
  lv_obj_t *volume_slider;
  lv_obj_t *volume_label;
} MusicUI;

extern MusicUI g_music_ui;

// Create all UI widgets and prepare the music list.
void music_ui_setup(MusicUI *ui);

// Drive the audio engine and process deferred UI actions.
void music_ui_loop(void);

#endif
