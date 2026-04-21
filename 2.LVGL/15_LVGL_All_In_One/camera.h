#ifndef __CAMERA_H
#define __CAMERA_H

#include "Arduino.h"
#include "esp_camera.h"

/***************************************************************************************
 * Camera Configuration Parameters
 ***************************************************************************************/
// Image resolution
#define CAMERA_FRAME_SIZE    FRAMESIZE_240X240  // 240x240 square frame
#define CAMERA_WIDTH         240                // Image width
#define CAMERA_HEIGHT        240                // Image height

// Image format
#define CAMERA_PIXEL_FORMAT  PIXFORMAT_RGB565   // RGB565 format (16-bit color)

// Frame buffer configuration
#define CAMERA_FB_COUNT      2                  // Double buffering for smoother output
#define CAMERA_FB_LOCATION   CAMERA_FB_IN_PSRAM // Store frame buffers in PSRAM

// Clock configuration
#define CAMERA_XCLK_FREQ     10000000           // 10MHz clock frequency

// JPEG quality (only applies in JPEG mode)
#define CAMERA_JPEG_QUALITY  10                 // Quality level (0-63, lower = higher quality)

/***************************************************************************************
 * ESP32-S3 Camera Pin Configuration
 ***************************************************************************************/
// Power and reset pins
#define PWDN_GPIO_NUM    -1   // Power control pin (unused)
#define RESET_GPIO_NUM   -1   // Reset pin (unused)

// Clock and I2C pins
#define XCLK_GPIO_NUM    15   // Master clock output
#define SIOD_GPIO_NUM    4    // I2C data line (SDA)
#define SIOC_GPIO_NUM    5    // I2C clock line (SCL)

// 8-bit parallel data pins
#define Y2_GPIO_NUM      11   // Data bit D0
#define Y3_GPIO_NUM      9    // Data bit D1
#define Y4_GPIO_NUM      8    // Data bit D2
#define Y5_GPIO_NUM      10   // Data bit D3
#define Y6_GPIO_NUM      12   // Data bit D4
#define Y7_GPIO_NUM      18   // Data bit D5
#define Y8_GPIO_NUM      17   // Data bit D6
#define Y9_GPIO_NUM      16   // Data bit D7

// Synchronization pins
#define VSYNC_GPIO_NUM   6    // Vertical sync signal
#define HREF_GPIO_NUM    7    // Horizontal reference signal
#define PCLK_GPIO_NUM    13   // Pixel clock

/***************************************************************************************
 * Camera Control Functions
 ***************************************************************************************/

/**
 * @brief Initialize the camera driver
 * @return true  on success
 * @return false on failure
 */
bool camera_init(void);

/**
 * @brief Set vertical flip state
 * @param state true=flipped, false=normal
 */
void camera_set_flip_vertical(bool state);

/**
 * @brief Set horizontal mirror state
 * @param state true=mirrored, false=normal
 */
void camera_set_mirror_horizontal(bool state);

/**
 * @brief Get current vertical flip state
 * @return true=flipped, false=normal
 */
bool camera_get_flip_vertical(void);

/**
 * @brief Get current horizontal mirror state
 * @return true=mirrored, false=normal
 */
bool camera_get_mirror_horizontal(void);

#endif
