#include "camera_ui.h"
#include "all_in_one_app.h"
#include "camera.h"
#include "img/img_index.h"
#include "sd_card.h"

#include "esp_heap_caps.h"

lv_img_dsc_t g_photo_display;
CameraUI g_camera_ui;

static TaskHandle_t s_camera_task_handle = NULL;
static volatile bool s_task_running = false;

static uint8_t *s_preview_buffers[2] = {NULL, NULL};
static volatile int s_display_buffer_index = 0;
static volatile int s_ready_buffer_index = -1;
static volatile bool s_frame_ready = false;
static portMUX_TYPE s_preview_lock = portMUX_INITIALIZER_UNLOCKED;

static bool camera_ui_allocate_preview_buffers() {
  for (int i = 0; i < 2; ++i) {
    if (s_preview_buffers[i] != NULL) {
      continue;
    }

    s_preview_buffers[i] = (uint8_t *)heap_caps_malloc(
      CAMERA_PREVIEW_BUFFER_SIZE,
      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );

    if (s_preview_buffers[i] == NULL) {
      s_preview_buffers[i] = (uint8_t *)heap_caps_malloc(
        CAMERA_PREVIEW_BUFFER_SIZE,
        MALLOC_CAP_8BIT
      );
    }

    if (s_preview_buffers[i] == NULL) {
      Serial.printf("Failed to allocate preview buffer %d\n", i);
      return false;
    }

    memset(s_preview_buffers[i], 0, CAMERA_PREVIEW_BUFFER_SIZE);
  }

  return true;
}

static void camera_ui_button_style_init(lv_obj_t *button) {
  lv_obj_set_style_bg_opa(button, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_opa(button, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_shadow_opa(button, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_outline_opa(button, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_pad_all(button, 0, LV_PART_MAIN);
}

static void camera_copy_rgb565_for_display(const camera_fb_t *frame, uint8_t *dst) {
  if (frame == NULL || frame->buf == NULL || dst == NULL) {
    return;
  }

  const uint8_t *src = frame->buf;
  const int width = min((int)frame->width, VIDEO_WIDTH);
  const int height = min((int)frame->height, VIDEO_HEIGHT);

  // 180° rotation: (x,y) -> (width-1-x, height-1-y)
  memset(dst, 0, CAMERA_PREVIEW_BUFFER_SIZE);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const size_t src_index = ((size_t)y * width + x) * 2;
      const size_t dst_index = ((size_t)y * VIDEO_WIDTH + x) * 2;

      // Swap RGB565 byte order while copying for LVGL/TFT display.
      dst[dst_index] = src[src_index + 1];
      dst[dst_index + 1] = src[src_index];
    }
  }
}

void camera_swap_rgb565_bytes(uint8_t *buffer, size_t length) {
  if (buffer == NULL || length == 0) {
    return;
  }

  for (size_t i = 0; i + 1 < length; i += 2) {
    const uint8_t temp = buffer[i];
    buffer[i] = buffer[i + 1];
    buffer[i + 1] = temp;
  }
}

void camera_task_loop(void *pvParameters) {
  (void)pvParameters;
  Serial.println("Camera preview task started");

  while (s_task_running) {
    camera_fb_t *frame = esp_camera_fb_get();
    if (frame != NULL && frame->buf != NULL) {
      int display_index = 0;
      taskENTER_CRITICAL(&s_preview_lock);
      display_index = s_display_buffer_index;
      taskEXIT_CRITICAL(&s_preview_lock);

      const int write_index = 1 - display_index;
      camera_copy_rgb565_for_display(frame, s_preview_buffers[write_index]);

      taskENTER_CRITICAL(&s_preview_lock);
      s_ready_buffer_index = write_index;
      s_frame_ready = true;
      taskEXIT_CRITICAL(&s_preview_lock);

      esp_camera_fb_return(frame);
    }

    vTaskDelay(pdMS_TO_TICKS(CAMERA_PREVIEW_FRAME_INTERVAL_MS));
  }

  taskENTER_CRITICAL(&s_preview_lock);
  s_ready_buffer_index = -1;
  s_frame_ready = false;
  s_camera_task_handle = NULL;
  taskEXIT_CRITICAL(&s_preview_lock);

  Serial.println("Camera preview task stopped");
  vTaskDelete(NULL);
}

void camera_task_start(void) {
  if (s_task_running) {
    Serial.println("Camera task is already running");
    return;
  }

  if (!camera_ui_allocate_preview_buffers()) {
    Serial.println("Failed to allocate preview buffers");
    return;
  }

  taskENTER_CRITICAL(&s_preview_lock);
  s_display_buffer_index = 0;
  s_ready_buffer_index = -1;
  s_frame_ready = false;
  taskEXIT_CRITICAL(&s_preview_lock);

  camera_ui_init_image_descriptor();
  s_task_running = true;

  BaseType_t result = xTaskCreate(
    camera_task_loop,
    "CameraPreview",
    CAMERA_TASK_STACK_SIZE,
    NULL,
    CAMERA_TASK_PRIORITY,
    &s_camera_task_handle
  );

  if (result != pdPASS) {
    Serial.println("Failed to create camera task");
    s_task_running = false;
    s_camera_task_handle = NULL;
  }
}

bool camera_task_stop(void) {
  if (!s_task_running) {
    return true;
  }

  s_task_running = false;

  uint32_t timeout_ms = TASK_STOP_TIMEOUT_MS;
  while (timeout_ms > 0) {
    taskENTER_CRITICAL(&s_preview_lock);
    const bool stopped = (s_camera_task_handle == NULL);
    taskEXIT_CRITICAL(&s_preview_lock);

    if (stopped) {
      Serial.println("Camera task stopped successfully");
      return true;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
    timeout_ms -= 10;
  }

  Serial.println("Camera task stop timeout");
  return false;
}

bool camera_task_is_running(void) {
  return s_task_running;
}

void camera_ui_refresh_preview(void) {
  int ready_index = -1;

  taskENTER_CRITICAL(&s_preview_lock);
  if (s_frame_ready) {
    ready_index = s_ready_buffer_index;
    s_display_buffer_index = ready_index;
    s_frame_ready = false;
  }
  taskEXIT_CRITICAL(&s_preview_lock);

  if (ready_index < 0 || ready_index > 1 || g_camera_ui.video_preview == NULL) {
    return;
  }

  g_photo_display.data = s_preview_buffers[ready_index];
  lv_obj_invalidate(g_camera_ui.video_preview);
}

static void on_photo_button_clicked(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  Serial.println("Taking photo...");

  if (s_task_running) {
    if (!camera_task_stop()) {
      Serial.println("Preview task did not stop cleanly");
      return;
    }
  }

  camera_fb_t *frame = esp_camera_fb_get();
  if (frame != NULL && frame->buf != NULL) {
    camera_swap_rgb565_bytes(frame->buf, frame->len);

    int photo_index = list_count_number(list_picture);
    if (photo_index != -1) {
      String file_path = String(PICTURE_FOLDER) + "/" + String(++photo_index) + ".bmp";
      write_rgb565_to_bmp(
        (char *)file_path.c_str(),
        frame->buf,
        frame->len,
        frame->height,
        frame->width
      );
      list_insert_tail(list_picture, (char *)file_path.c_str());
      Serial.printf("Photo saved: %s\n", file_path.c_str());
    }

    esp_camera_fb_return(frame);
  } else {
    Serial.println("Camera capture failed");
  }

  camera_task_start();
}

static void on_exit_button_clicked(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  all_in_one_show_home();
}

static void on_screen_gesture(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_GESTURE) {
    return;
  }

  const lv_dir_t direction = lv_indev_get_gesture_dir(lv_indev_get_act());

  switch (direction) {
    case LV_DIR_LEFT:
    case LV_DIR_RIGHT:
      camera_set_mirror_horizontal(!camera_get_mirror_horizontal());
      break;

    case LV_DIR_TOP:
    case LV_DIR_BOTTOM:
      camera_set_flip_vertical(!camera_get_flip_vertical());
      break;

    default:
      break;
  }
}

void camera_ui_init_image_descriptor(void) {
  lv_img_header_t header;
  header.always_zero = 0;
  header.w = VIDEO_WIDTH;
  header.h = VIDEO_HEIGHT;
  header.cf = LV_IMG_CF_TRUE_COLOR;

  g_photo_display.header = header;
  g_photo_display.data_size = CAMERA_PREVIEW_BUFFER_SIZE;
  g_photo_display.data = s_preview_buffers[0];
}

void camera_ui_setup(CameraUI *ui) {
  Serial.println("Setting up camera UI...");

  ui->screen = lv_obj_create(NULL);

  setup_list_head_picture();

  static lv_style_t bg_style;
  lv_style_init(&bg_style);
  lv_style_set_bg_color(&bg_style, lv_color_hex(0xFFFFFF));
  lv_obj_add_style(ui->screen, &bg_style, LV_PART_MAIN);

  static lv_style_t btn_pressed_style;
  lv_style_init(&btn_pressed_style);
  lv_style_set_translate_y(&btn_pressed_style, BTN_PRESS_OFFSET);

  ui->video_preview = lv_img_create(ui->screen);
  lv_obj_set_pos(ui->video_preview, VIDEO_X, VIDEO_Y);
  lv_obj_set_size(ui->video_preview, VIDEO_WIDTH, VIDEO_HEIGHT);

  if (camera_ui_allocate_preview_buffers()) {
    camera_ui_init_image_descriptor();
    lv_img_set_src(ui->video_preview, &g_photo_display);
  }

  ui->photo_button = lv_img_create(ui->screen);
  lv_obj_set_pos(ui->photo_button, PHOTO_BTN_X, PHOTO_BTN_Y);
  lv_obj_set_size(ui->photo_button, PHOTO_BTN_WIDTH, PHOTO_BTN_HEIGHT);
  lv_img_set_src(ui->photo_button, GALLERY_IMG_CAM);
  lv_obj_add_flag(ui->photo_button, LV_OBJ_FLAG_CLICKABLE);
  camera_ui_button_style_init(ui->photo_button);
  lv_obj_add_style(ui->photo_button, &btn_pressed_style, LV_STATE_PRESSED);

  ui->exit_button = lv_img_create(ui->screen);
  lv_obj_set_pos(ui->exit_button, EXIT_BTN_X, EXIT_BTN_Y);
  lv_obj_set_size(ui->exit_button, EXIT_BTN_SIZE, EXIT_BTN_SIZE);
  lv_img_set_src(ui->exit_button, GALLERY_IMG_ESC);
  lv_obj_add_flag(ui->exit_button, LV_OBJ_FLAG_CLICKABLE);
  camera_ui_button_style_init(ui->exit_button);
  lv_obj_add_style(ui->exit_button, &btn_pressed_style, LV_STATE_PRESSED);

  lv_obj_add_event_cb(ui->photo_button, on_photo_button_clicked, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(ui->exit_button, on_exit_button_clicked, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(ui->screen, on_screen_gesture, LV_EVENT_ALL, NULL);

  camera_task_start();
  Serial.println("Camera UI setup complete");
}
