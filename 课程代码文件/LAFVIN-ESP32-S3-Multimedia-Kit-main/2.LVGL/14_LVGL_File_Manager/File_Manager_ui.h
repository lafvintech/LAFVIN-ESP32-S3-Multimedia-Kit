#ifndef __FILE_MANAGER_UI_H
#define __FILE_MANAGER_UI_H

#include "Arduino.h"
#include "lvgl.h"

typedef struct {
  lv_obj_t *screen;
  lv_obj_t *exit_button;
  lv_obj_t *wifi_badge;
  lv_obj_t *title_label;
  lv_obj_t *path_label;
  lv_obj_t *list;
  lv_obj_t *status_label;

  lv_obj_t *overlay;
  lv_obj_t *dialog;
  lv_obj_t *dialog_title;
  lv_obj_t *dialog_info;
  lv_obj_t *cancel_button;
  lv_obj_t *delete_button;
} FileManagerUI;

extern FileManagerUI g_file_manager_ui;

void file_manager_ui_setup(FileManagerUI *ui);
void file_manager_ui_loop(void);

#endif
