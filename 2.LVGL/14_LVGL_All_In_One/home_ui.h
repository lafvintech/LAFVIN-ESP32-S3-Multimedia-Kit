#ifndef __HOME_UI_H
#define __HOME_UI_H

#include "Arduino.h"
#include "lvgl.h"

// Home screen object bundle used by the app shell.
typedef struct {
  lv_obj_t *screen;
  lv_obj_t *tileview;
  lv_obj_t *page_one;
  lv_obj_t *page_two;
  lv_obj_t *time_label_page_one;
  lv_obj_t *time_label_page_two;
} HomeUI;

extern HomeUI g_home_ui;

// Build the two-page home screen.
void home_ui_setup(HomeUI *ui);

// Switch between the two fixed home pages.
void home_ui_show_page(uint8_t page_index, bool animate);

// Update the clock shown in the home header.
void home_ui_set_time_text(const char *text);

// Read back the currently visible home page.
uint8_t home_ui_get_page(void);

#endif
