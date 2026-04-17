/***************************************************************************************
 * ESP32-S3 摄像头应用 (ESP32-S3 Camera Application)
 * 
 * 功能说明 (Features):
 * - 实时摄像头预览 (Live camera preview)
 * - 拍照并保存到 SD 卡 (Take photos and save to SD card)
 * - 手势控制图像翻转/镜像 (Gesture control for flip/mirror)
 * 
 * 硬件要求 (Hardware Requirements):
 * - ESP32-S3 开发板 (ESP32-S3 Dev Board)
 * - 摄像头模块 (Camera Module)
 * - TFT 显示屏 240x320 (TFT Display 240x320)
 * - SD 卡 (SD Card)
 * 
 * 操作说明 (Instructions):
 * - 点击底部按钮拍照 (Tap bottom button to take photo)
 * - 左右滑动切换镜像 (Swipe left/right to toggle mirror)
 * - 上下滑动切换翻转 (Swipe up/down to toggle flip)
 ***************************************************************************************/

#include "display.h"
#include <lvgl.h>
#include "sd_card.h"
#include "camera.h"
#include "camera_ui.h"

/***************************************************************************************
 * 全局对象 (Global Objects)
 ***************************************************************************************/
Display screen;  // 显示屏管理对象

/***************************************************************************************
 * 初始化函数 (Setup Function)
 ***************************************************************************************/
void setup() {
  // 初始化串口 (波特率: 115200)
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n========================================");
  Serial.println("  ESP32-S3 Camera Application");
  Serial.println("========================================\n");
  
  // 1. 初始化 SD 卡
  Serial.println("[1/4] Initializing SD card...");
  if (!sdcard_init()) {
    Serial.println("✗ SD card init failed!");
  } else {
    Serial.println("✓ SD card ready\n");
  }
  
  // 2. 初始化摄像头
  Serial.println("[2/4] Initializing camera...");
  if (!camera_init()) {
    Serial.println("✗ Camera init failed!");
    Serial.println("System halted. Please check hardware.");
    while(1) { delay(1000); }  // 停止运行
  }
  Serial.println();
  
  // 3. 初始化显示屏和 LVGL
  Serial.println("[3/4] Initializing display...");
  screen.init();
  
  // 打印 LVGL 版本信息
  Serial.printf("✓ LVGL v%d.%d.%d initialized\n\n",
    lv_version_major(),
    lv_version_minor(),
    lv_version_patch()
  );
  
  // 4. 初始化摄像头 UI
  Serial.println("[4/4] Setting up camera UI...");
  camera_ui_setup(&g_camera_ui);
  lv_scr_load(g_camera_ui.screen);
  Serial.println();
  
  Serial.println("========================================");
  Serial.println("  System Ready!");
  Serial.println("========================================");
  Serial.println("Tips:");
  Serial.println("  • Tap camera button to take photo");
  Serial.println("  • Swipe left/right to toggle mirror");
  Serial.println("  • Swipe up/down to toggle flip");
  Serial.println("========================================\n");
}

/***************************************************************************************
 * 主循环函数 (Main Loop Function)
 ***************************************************************************************/
void loop() {
  camera_ui_refresh_preview();
  // 处理 LVGL 任务 (动画、事件、刷新等)
  screen.routine();
  
  // 短暂延时,推荐 5ms
  delay(5);
}
