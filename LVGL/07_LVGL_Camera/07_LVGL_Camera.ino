/***************************************************************************************
 * ESP32-S3 Camera Application
 *
 * Features:
 * - Live camera preview
 * - Take photos and save them to the SD card
 * - Gesture control for image flip and mirror
 *
 * Hardware Requirements:
 * - ESP32-S3 development board
 * - Camera module
 * - 240x320 TFT display
 * - SD card
 *
 * Instructions:
 * - Tap the bottom button to take a photo
 * - Swipe left/right to toggle mirror
 * - Swipe up/down to toggle flip
 ***************************************************************************************/

#include "display.h"
#include <lvgl.h>
#include "sd_card.h"
#include "camera.h"
#include "camera_ui.h"

/***************************************************************************************
 * Global Objects
 ***************************************************************************************/
Display screen;  // Display manager object

/***************************************************************************************
 * Setup Function
 ***************************************************************************************/
void setup() {
  // Initialize the serial port (baud rate 115200)
  Serial.begin(115200);
  delay(100);

  Serial.println("\n========================================");
  Serial.println("  ESP32-S3 Camera Application");
  Serial.println("========================================\n");

  // 1. Initialize the SD card
  Serial.println("[1/4] Initializing SD card...");
  if (!sdcard_init()) {
    Serial.println("鉁?SD card init failed!");
  } else {
    Serial.println("鉁?SD card ready\n");
  }

  // 2. Initialize the camera
  Serial.println("[2/4] Initializing camera...");
  if (!camera_init()) {
    Serial.println("鉁?Camera init failed!");
    Serial.println("System halted. Please check hardware.");
    while(1) { delay(1000); }  // Stop execution
  }
  Serial.println();

  // 3. Initialize the display and LVGL
  Serial.println("[3/4] Initializing display...");
  screen.init();

  // Print LVGL version information
  Serial.printf("鉁?LVGL v%d.%d.%d initialized\n\n",
    lv_version_major(),
    lv_version_minor(),
    lv_version_patch()
  );

  // 4. Initialize the camera UI
  Serial.println("[4/4] Setting up camera UI...");
  camera_ui_setup(&g_camera_ui);
  lv_scr_load(g_camera_ui.screen);
  Serial.println();

  Serial.println("========================================");
  Serial.println("  System Ready!");
  Serial.println("========================================");
  Serial.println("Tips:");
  Serial.println("  鈥?Tap camera button to take photo");
  Serial.println("  鈥?Swipe left/right to toggle mirror");
  Serial.println("  鈥?Swipe up/down to toggle flip");
  Serial.println("========================================\n");
}

/***************************************************************************************
 * Main Loop Function
 ***************************************************************************************/
void loop() {
  camera_ui_refresh_preview();
  // Handle LVGL tasks such as animations, events, and screen refresh
  screen.routine();

  // Short delay, 5 ms recommended
  delay(5);
}
