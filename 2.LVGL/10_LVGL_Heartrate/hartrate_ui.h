#ifndef __HARTRATE_UI_H
#define __HARTRATE_UI_H

#include "Arduino.h"
#include "lvgl.h"

// Board-level I2C pins for MAX30102.
#define HEARTRATE_I2C_SDA 2
#define HEARTRATE_I2C_SCL 1

// Display range used by the waveform chart.
#define CHART_LOW_LIMIT 0
#define CHART_HIGH_LIMIT 2000
#define CHART_POINT_COUNT 90

// LVGL objects used by the heart-rate page.
typedef struct {
  lv_obj_t *screen;
  lv_obj_t *exit_button;
  lv_obj_t *exit_label;
  lv_obj_t *title_label;
  lv_obj_t *bpm_label;
  lv_obj_t *status_label;
  lv_obj_t *chart;
} HeartrateUI;

extern HeartrateUI g_heartrate_ui;

// Build all widgets and start heart-rate measurement.
void hartrate_ui_setup(HeartrateUI *ui);

// Start or resume heart-rate measurement when entering the page.
void hartrate_ui_start(void);

// Refresh labels/debug output on each main loop tick.
void hartrate_ui_loop(void);

// Stop the background sampling task and shut down the sensor.
void hartrate_ui_stop(void);

#endif
