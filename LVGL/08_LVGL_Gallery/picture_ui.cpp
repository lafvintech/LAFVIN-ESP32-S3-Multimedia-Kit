#include "picture_ui.h"
#include "sd_card.h"
#include "img/img_index.h"

#include "esp_heap_caps.h"

lvgl_picture_ui guider_picture_ui;
static int picture_index_num = 1;

// Runtime image descriptor used for the currently displayed BMP.
static lv_img_dsc_t s_bmp_img_dsc;
static uint8_t *s_bmp_pixels = NULL;
static bool s_bmp_ready = false;

static uint16_t read_u16_le(File &file) {
  uint8_t bytes[2] = {0};
  file.read(bytes, 2);
  return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8);
}

static uint32_t read_u32_le(File &file) {
  uint8_t bytes[4] = {0};
  file.read(bytes, 4);
  return (uint32_t)bytes[0]
       | ((uint32_t)bytes[1] << 8)
       | ((uint32_t)bytes[2] << 16)
       | ((uint32_t)bytes[3] << 24);
}

static int32_t read_i32_le(File &file) {
  return (int32_t)read_u32_le(file);
}

// Allocate a reusable RGB565 buffer for SD card BMP decoding.
static bool picture_buffer_init() {
  if (s_bmp_pixels != NULL) {
    return true;
  }

  const size_t image_size = 240 * 240 * 2;
  s_bmp_pixels = (uint8_t *)heap_caps_malloc(image_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (s_bmp_pixels == NULL) {
    s_bmp_pixels = (uint8_t *)heap_caps_malloc(image_size, MALLOC_CAP_8BIT);
  }

  if (s_bmp_pixels == NULL) {
    Serial.println("Failed to allocate BMP buffer");
    return false;
  }

  memset(s_bmp_pixels, 0, image_size);

  lv_img_header_t header;
  header.always_zero = 0;
  header.w = 240;
  header.h = 240;
  header.cf = LV_IMG_CF_TRUE_COLOR;

  s_bmp_img_dsc.header = header;
  s_bmp_img_dsc.data_size = image_size;
  s_bmp_img_dsc.data = s_bmp_pixels;
  s_bmp_ready = true;
  return true;
}

// Load a 240x240 16-bit BMP from /picture into the shared buffer.
static bool load_bmp_from_sd(const char *name) {
  if (name == NULL || !picture_buffer_init()) {
    return false;
  }

  String file_path = String("/picture/") + name;
  File file = SD_MMC.open(file_path.c_str(), FILE_READ);
  if (!file) {
    Serial.printf("Failed to open BMP: %s\n", file_path.c_str());
    return false;
  }

  const uint16_t signature = read_u16_le(file);
  if (signature != 0x4D42) {
    Serial.printf("Invalid BMP signature: %s\n", file_path.c_str());
    file.close();
    return false;
  }

  (void)read_u32_le(file);  // file size
  (void)read_u32_le(file);  // reserved
  const uint32_t data_offset = read_u32_le(file);
  const uint32_t header_size = read_u32_le(file);
  const int32_t width = read_i32_le(file);
  const int32_t height = read_i32_le(file);
  const uint16_t planes = read_u16_le(file);
  const uint16_t bit_count = read_u16_le(file);
  const uint32_t compression = read_u32_le(file);
  (void)read_u32_le(file);  // image size
  (void)read_i32_le(file);  // x pixels per meter
  (void)read_i32_le(file);  // y pixels per meter
  (void)read_u32_le(file);  // colors used
  (void)read_u32_le(file);  // colors important

  if (header_size < 40 || planes != 1 || bit_count != 16) {
    Serial.printf("Unsupported BMP format: %s\n", file_path.c_str());
    file.close();
    return false;
  }

  if (width != 240 || (height != 240 && height != -240)) {
    Serial.printf("Unsupported BMP size: %ldx%ld\n", (long)width, (long)height);
    file.close();
    return false;
  }

  // Support BI_RGB (0) and BI_BITFIELDS (3), which matches the camera-generated BMP.
  if (compression != 0 && compression != 3) {
    Serial.printf("Unsupported BMP compression: %lu\n", (unsigned long)compression);
    file.close();
    return false;
  }

  const bool bottom_up = (height > 0);
  const uint32_t row_size = 240 * 2;
  uint8_t row_buffer[row_size];

  for (int row = 0; row < 240; ++row) {
    const uint32_t bmp_row = bottom_up ? (239 - row) : row;
    if (!file.seek(data_offset + bmp_row * row_size)) {
      Serial.printf("Seek failed for row %d\n", row);
      file.close();
      return false;
    }

    const size_t read_len = file.read(row_buffer, row_size);
    if (read_len != row_size) {
      Serial.printf("Short read on row %d\n", row);
      file.close();
      return false;
    }

    memcpy(s_bmp_pixels + row * row_size, row_buffer, row_size);
  }

  file.close();
  return true;
}

static void picture_imgbtn_left_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    Serial.println("Clicked the left button.");
    picture_index_num--;
    if (picture_index_num < 1) {
      picture_index_num = list_count_number(list_picture);
    }
    picture_imgbtn_display(list_find_node(list_picture, picture_index_num));
  }
}

static void picture_imgbtn_right_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    Serial.println("Clicked the right button.");
    picture_index_num++;
    if (picture_index_num > list_count_number(list_picture)) {
      picture_index_num = 1;
    }
    picture_imgbtn_display(list_find_node(list_picture, picture_index_num));
  }
}

static void picture_imgbtn_home_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    Serial.println("Clicked the ESC button. Placeholder action.");
  }
}

// Layout for the landscape gallery page.
static constexpr lv_coord_t kHomeButtonSize = 32;
static constexpr lv_coord_t kHomeButtonInset = 8;
static constexpr lv_coord_t kHomeButtonY = 8;
static constexpr lv_coord_t kImageX = 40;
static constexpr lv_coord_t kImageY = 0;
static constexpr lv_coord_t kImageSize = 240;
static constexpr lv_coord_t kButtonSize = 60;
static constexpr lv_coord_t kButtonInset = 2;
static constexpr lv_coord_t kButtonY = (240 - kButtonSize) / 2;
static constexpr lv_coord_t kLeftButtonX = kButtonInset;
static constexpr lv_coord_t kRightButtonX = 320 - kButtonSize - kButtonInset;
static constexpr lv_coord_t kHomeButtonX = kHomeButtonInset;

// Remove the default LVGL button background so only the icon is visible.
static void picture_button_style_init(lv_obj_t *button) {
  lv_obj_set_style_bg_opa(button, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_opa(button, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_shadow_opa(button, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_outline_opa(button, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_pad_all(button, 0, LV_PART_MAIN);
}

void setup_scr_picture(lvgl_picture_ui *ui) {
  ui->picture = lv_obj_create(NULL);

  static lv_style_t bg_style;
  lv_style_init(&bg_style);
  lv_style_set_bg_color(&bg_style, lv_color_hex(0xffffff));
  lv_obj_add_style(ui->picture, &bg_style, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui->picture, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui->picture, 0, LV_PART_MAIN);

  static lv_style_t style_pr;
  lv_style_init(&style_pr);
  lv_style_set_translate_y(&style_pr, 5);

  // Create the picture first, then place buttons above it.
  ui->picture_show = lv_img_create(ui->picture);
  lv_obj_set_pos(ui->picture_show, kImageX, kImageY);
  lv_obj_set_size(ui->picture_show, kImageSize, kImageSize);

  ui->picture_home = lv_imgbtn_create(ui->picture);
  lv_obj_set_pos(ui->picture_home, kHomeButtonX, kHomeButtonY);
  lv_obj_set_size(ui->picture_home, kHomeButtonSize, kHomeButtonSize);
  lv_imgbtn_set_src(ui->picture_home, LV_IMGBTN_STATE_RELEASED, NULL, GALLERY_IMG_ESC, NULL);
  lv_imgbtn_set_src(ui->picture_home, LV_IMGBTN_STATE_PRESSED, NULL, GALLERY_IMG_ESC, NULL);
  picture_button_style_init(ui->picture_home);
  lv_obj_add_style(ui->picture_home, &style_pr, LV_STATE_PRESSED);

  ui->picture_left = lv_imgbtn_create(ui->picture);
  lv_obj_set_pos(ui->picture_left, kLeftButtonX, kButtonY);
  lv_obj_set_size(ui->picture_left, kButtonSize, kButtonSize);
  lv_imgbtn_set_src(ui->picture_left, LV_IMGBTN_STATE_RELEASED, NULL, GALLERY_IMG_LEFT, NULL);
  lv_imgbtn_set_src(ui->picture_left, LV_IMGBTN_STATE_PRESSED, NULL, GALLERY_IMG_LEFT, NULL);
  picture_button_style_init(ui->picture_left);
  lv_obj_add_style(ui->picture_left, &style_pr, LV_STATE_PRESSED);

  ui->picture_right = lv_imgbtn_create(ui->picture);
  lv_obj_set_pos(ui->picture_right, kRightButtonX, kButtonY);
  lv_obj_set_size(ui->picture_right, kButtonSize, kButtonSize);
  lv_imgbtn_set_src(ui->picture_right, LV_IMGBTN_STATE_RELEASED, NULL, GALLERY_IMG_RIGHT, NULL);
  lv_imgbtn_set_src(ui->picture_right, LV_IMGBTN_STATE_PRESSED, NULL, GALLERY_IMG_RIGHT, NULL);
  picture_button_style_init(ui->picture_right);
  lv_obj_add_style(ui->picture_right, &style_pr, LV_STATE_PRESSED);

  setup_list_head_picture();
  picture_index_num = list_count_number(list_picture);
  picture_imgbtn_display(list_find_node(list_picture, picture_index_num));

  lv_obj_add_event_cb(ui->picture_left, picture_imgbtn_left_event_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(ui->picture_right, picture_imgbtn_right_event_handler, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(ui->picture_home, picture_imgbtn_home_event_handler, LV_EVENT_ALL, NULL);
}

// Update the gallery preview with the selected BMP file from the SD card.
void picture_imgbtn_display(char *name) {
  if (name == NULL) {
    lv_img_set_src(guider_picture_ui.picture_show, NULL);
    Serial.println("No pictures found in folder");
    return;
  }

  Serial.printf("Loading image: %s\n", name);
  const unsigned long start_time = millis();

  if (!load_bmp_from_sd(name)) {
    lv_img_set_src(guider_picture_ui.picture_show, NULL);
    Serial.println("Failed to decode BMP");
    return;
  }

  if (s_bmp_ready) {
    lv_img_set_src(guider_picture_ui.picture_show, &s_bmp_img_dsc);
    lv_obj_invalidate(guider_picture_ui.picture_show);
    lv_refr_now(NULL);
  }

  const unsigned long load_time = millis() - start_time;
  Serial.printf("Image loaded in %lu ms\n", load_time);
}
