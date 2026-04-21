#ifndef __WIFI_UI_H
#define __WIFI_UI_H

#include "Arduino.h"
#include "lvgl.h"

typedef struct {
  lv_obj_t *screen;
  lv_obj_t *exit_button;
  lv_obj_t *refresh_button;
  lv_obj_t *title_label;

  lv_obj_t *wifi_list;
  lv_obj_t *status_label;
  lv_obj_t *ip_label;

  lv_obj_t *overlay;
  lv_obj_t *dialog;
  lv_obj_t *dialog_title;
  lv_obj_t *ssid_label;
  lv_obj_t *password_area;
  lv_obj_t *cancel_button;
  lv_obj_t *connect_button;
  lv_obj_t *keyboard;
} WifiUI;

extern WifiUI g_wifi_ui;

void wifi_ui_setup(WifiUI *ui);
void wifi_ui_loop(void);

#endif
