#include "camera.h"

/***************************************************************************************
 * Static Variables
 ***************************************************************************************/
static bool g_flip_vertical_enabled = true;      // Vertical flip state
static bool g_mirror_horizontal_enabled = true;  // Horizontal mirror state

/***************************************************************************************
 * Camera Initialization
 ***************************************************************************************/
bool camera_init(void) {
  Serial.println("Initializing camera...");

  // Configure camera parameters using struct initializer list
  camera_config_t config = {
    .pin_pwdn = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk = XCLK_GPIO_NUM,
    .pin_sccb_sda = SIOD_GPIO_NUM,
    .pin_sccb_scl = SIOC_GPIO_NUM,

    .pin_d7 = Y9_GPIO_NUM,
    .pin_d6 = Y8_GPIO_NUM,
    .pin_d5 = Y7_GPIO_NUM,
    .pin_d4 = Y6_GPIO_NUM,
    .pin_d3 = Y5_GPIO_NUM,
    .pin_d2 = Y4_GPIO_NUM,
    .pin_d1 = Y3_GPIO_NUM,
    .pin_d0 = Y2_GPIO_NUM,

    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href = HREF_GPIO_NUM,
    .pin_pclk = PCLK_GPIO_NUM,

    .xclk_freq_hz = CAMERA_XCLK_FREQ,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = CAMERA_PIXEL_FORMAT,
    .frame_size = CAMERA_FRAME_SIZE,

    .jpeg_quality = CAMERA_JPEG_QUALITY,
    .fb_count = CAMERA_FB_COUNT,
    .fb_location = CAMERA_FB_LOCATION,
    .grab_mode = CAMERA_GRAB_LATEST
  };

  // Initialize camera driver
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("✗ Camera init failed! Error code: 0x%x\n", err);

    // Print detailed error info
    switch(err) {
      case ESP_ERR_NO_MEM:
        Serial.println("  Reason: Out of memory");
        break;
      case ESP_ERR_NOT_FOUND:
        Serial.println("  Reason: Camera not found");
        break;
      case ESP_ERR_NOT_SUPPORTED:
        Serial.println("  Reason: Configuration not supported");
        break;
      default:
        Serial.println("  Reason: Unknown error");
        break;
    }
    return false;
  }

  // Get sensor control handle
  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor == NULL) {
    Serial.println("✗ Failed to get camera sensor!");
    return false;
  }

  // Configure image parameters
  sensor->set_vflip(sensor, g_flip_vertical_enabled);        // Vertical flip
  sensor->set_hmirror(sensor, g_mirror_horizontal_enabled);  // Horizontal mirror
  sensor->set_brightness(sensor, 0);                         // Brightness (range: -2 to 2)
  sensor->set_saturation(sensor, 0);                         // Saturation (range: -2 to 2)

  Serial.println("✓ Camera initialized successfully!");
  Serial.printf("  Resolution: %dx%d\n", CAMERA_WIDTH, CAMERA_HEIGHT);
  Serial.printf("  Format: RGB565\n");
  Serial.printf("  Frame buffers: %d\n", CAMERA_FB_COUNT);

  return true;
}

/***************************************************************************************
 * Flip & Mirror Control
 ***************************************************************************************/

bool camera_get_flip_vertical(void) {
  return g_flip_vertical_enabled;
}

bool camera_get_mirror_horizontal(void) {
  return g_mirror_horizontal_enabled;
}

void camera_set_flip_vertical(bool state) {
  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor != NULL) {
    g_flip_vertical_enabled = state;
    sensor->set_vflip(sensor, g_flip_vertical_enabled);
    Serial.printf("Vertical flip: %s\n", state ? "ON" : "OFF");
  }
}

void camera_set_mirror_horizontal(bool state) {
  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor != NULL) {
    g_mirror_horizontal_enabled = state;
    sensor->set_hmirror(sensor, g_mirror_horizontal_enabled);
    Serial.printf("Horizontal mirror: %s\n", state ? "ON" : "OFF");
  }
}
