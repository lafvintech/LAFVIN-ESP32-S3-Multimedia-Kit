#include "camera.h"

/***************************************************************************************
 * 静态变量 (Static Variables)
 ***************************************************************************************/
static bool g_flip_vertical_enabled = true;      // 垂直翻转状态
static bool g_mirror_horizontal_enabled = true;  // 水平镜像状态

/***************************************************************************************
 * 摄像头初始化函数 (Camera Initialization)
 ***************************************************************************************/
bool camera_init(void) {
  Serial.println("Initializing camera...");
  
  // 配置摄像头参数 (使用结构体初始化列表)
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
  
  // 初始化摄像头驱动
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("✗ Camera init failed! Error code: 0x%x\n", err);
    
    // 打印详细错误信息
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

  // 获取传感器控制句柄
  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor == NULL) {
    Serial.println("✗ Failed to get camera sensor!");
    return false;
  }
  
  // 配置图像参数
  sensor->set_vflip(sensor, g_flip_vertical_enabled);        // 垂直翻转
  sensor->set_hmirror(sensor, g_mirror_horizontal_enabled);  // 水平镜像
  sensor->set_brightness(sensor, 0);                         // 亮度 (范围: -2 到 2)
  sensor->set_saturation(sensor, 0);                         // 饱和度 (范围: -2 到 2)

  Serial.println("✓ Camera initialized successfully!");
  Serial.printf("  Resolution: %dx%d\n", CAMERA_WIDTH, CAMERA_HEIGHT);
  Serial.printf("  Format: RGB565\n");
  Serial.printf("  Frame buffers: %d\n", CAMERA_FB_COUNT);
  
  return true;
}

/***************************************************************************************
 * 图像翻转/镜像控制函数 (Flip & Mirror Control)
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
