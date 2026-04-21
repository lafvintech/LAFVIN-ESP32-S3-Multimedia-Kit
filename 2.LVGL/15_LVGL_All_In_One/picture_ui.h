#ifndef __PICTURE_UI_H
#define __PICTURE_UI_H

#include "lvgl.h"
#include "Arduino.h"

// Gallery screen objects used by the sketch.
typedef struct lvgl_picture {
  lv_obj_t *picture;
  lv_obj_t *picture_home;
  lv_obj_t *picture_left;
  lv_obj_t *picture_right;
  lv_obj_t *picture_show;
} lvgl_picture_ui;

extern lvgl_picture_ui guider_picture_ui;

void setup_scr_picture(lvgl_picture_ui *ui);
void picture_imgbtn_display(char *name);

#endif
