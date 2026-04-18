#ifndef __CAMERA_UI_H
#define __CAMERA_UI_H

#include "Arduino.h"
#include "esp_camera.h"
#include "lvgl.h"

// Screen dimensions
#define SCREEN_WIDTH         320
#define SCREEN_HEIGHT        240

// Video preview area
#define VIDEO_X              0
#define VIDEO_Y              0
#define VIDEO_WIDTH          240
#define VIDEO_HEIGHT         240

// Photo button
#define PHOTO_BTN_WIDTH      80
#define PHOTO_BTN_HEIGHT     80
#define PHOTO_BTN_X          240
#define PHOTO_BTN_Y          ((SCREEN_HEIGHT - PHOTO_BTN_HEIGHT) / 2)

// Exit button
#define EXIT_BTN_SIZE        32
#define EXIT_BTN_X           (SCREEN_WIDTH - EXIT_BTN_SIZE - 8)
#define EXIT_BTN_Y           8

// Button press animation
#define BTN_PRESS_OFFSET     5

// Task configuration
#define CAMERA_TASK_STACK_SIZE           8192
#define CAMERA_TASK_PRIORITY             1
#define TASK_STOP_TIMEOUT_MS             1000
#define CAMERA_PREVIEW_FRAME_INTERVAL_MS 20
#define CAMERA_PREVIEW_BUFFER_SIZE       (VIDEO_WIDTH * VIDEO_HEIGHT * 2)

typedef struct {
  lv_obj_t *screen;
  lv_obj_t *video_preview;
  lv_obj_t *photo_button;
  lv_obj_t *exit_button;
} CameraUI;

extern lv_img_dsc_t g_photo_display;
extern CameraUI g_camera_ui;

void camera_task_start(void);
bool camera_task_stop(void);
void camera_task_loop(void *pvParameters);
bool camera_task_is_running(void);

void camera_ui_init_image_descriptor(void);
void camera_ui_setup(CameraUI *ui);
void camera_ui_refresh_preview(void);

void camera_swap_rgb565_bytes(uint8_t *buffer, size_t length);

#endif
