#ifndef __CAMERA_H
#define __CAMERA_H

#include "Arduino.h"
#include "esp_camera.h"

/***************************************************************************************
 * 摄像头配置参数 (Camera Configuration Parameters)
 ***************************************************************************************/
// 图像分辨率配置 (Image Resolution)
#define CAMERA_FRAME_SIZE    FRAMESIZE_240X240  // 240x240 方形画面
#define CAMERA_WIDTH         240                // 图像宽度
#define CAMERA_HEIGHT        240                // 图像高度

// 图像格式配置 (Image Format)
#define CAMERA_PIXEL_FORMAT  PIXFORMAT_RGB565   // RGB565 格式 (16位色)

// 帧缓冲配置 (Frame Buffer Configuration)
#define CAMERA_FB_COUNT      2                  // 双缓冲,提高流畅度
#define CAMERA_FB_LOCATION   CAMERA_FB_IN_PSRAM // 使用 PSRAM 存储帧缓冲

// 时钟配置 (Clock Configuration)
#define CAMERA_XCLK_FREQ     10000000           // 10MHz 时钟频率

// JPEG 质量 (仅在 JPEG 模式下有效)
#define CAMERA_JPEG_QUALITY  10                 // 质量等级 (0-63, 越小质量越高)

/***************************************************************************************
 * ESP32-S3 摄像头引脚配置 (Camera Pin Configuration)
 ***************************************************************************************/
// 电源和复位引脚 (Power & Reset Pins)
#define PWDN_GPIO_NUM    -1   // 电源控制引脚 (未使用)
#define RESET_GPIO_NUM   -1   // 复位引脚 (未使用)

// 时钟和 I2C 引脚 (Clock & I2C Pins)
#define XCLK_GPIO_NUM    15   // 主时钟输出
#define SIOD_GPIO_NUM    4    // I2C 数据线 (SDA)
#define SIOC_GPIO_NUM    5    // I2C 时钟线 (SCL)

// 8位并行数据引脚 (8-bit Parallel Data Pins)
#define Y2_GPIO_NUM      11   // 数据位 D0
#define Y3_GPIO_NUM      9    // 数据位 D1
#define Y4_GPIO_NUM      8    // 数据位 D2
#define Y5_GPIO_NUM      10   // 数据位 D3
#define Y6_GPIO_NUM      12   // 数据位 D4
#define Y7_GPIO_NUM      18   // 数据位 D5
#define Y8_GPIO_NUM      17   // 数据位 D6
#define Y9_GPIO_NUM      16   // 数据位 D7

// 同步信号引脚 (Synchronization Pins)
#define VSYNC_GPIO_NUM   6    // 垂直同步信号
#define HREF_GPIO_NUM    7    // 水平参考信号
#define PCLK_GPIO_NUM    13   // 像素时钟

/***************************************************************************************
 * 摄像头控制函数 (Camera Control Functions)
 ***************************************************************************************/

/**
 * @brief 初始化摄像头驱动
 * @return true  初始化成功
 * @return false 初始化失败
 */
bool camera_init(void);

/**
 * @brief 设置垂直翻转状态
 * @param state true=翻转, false=正常
 */
void camera_set_flip_vertical(bool state);

/**
 * @brief 设置水平镜像状态
 * @param state true=镜像, false=正常
 */
void camera_set_mirror_horizontal(bool state);

/**
 * @brief 获取当前垂直翻转状态
 * @return true=已翻转, false=正常
 */
bool camera_get_flip_vertical(void);

/**
 * @brief 获取当前水平镜像状态
 * @return true=已镜像, false=正常
 */
bool camera_get_mirror_horizontal(void);

#endif