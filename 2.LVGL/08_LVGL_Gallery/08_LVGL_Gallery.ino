/*
 * ESP32-S3 Picture Gallery
 *
 * Features:
 * - Load BMP images from the SD card
 * - Switch pictures with left/right overlay buttons
 * - Show an ESC button at the top-left corner
 */

#include "display.h"
#include <lvgl.h>
#include "sd_card.h"
#include "picture_ui.h"

Display screen;

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n========================================");
  Serial.println("  ESP32-S3 Picture Gallery");
  Serial.println("========================================\n");

  // 1. Mount the SD card first because gallery images come from /picture.
  Serial.println("[1/3] Initializing SD card...");
  if (!sdcard_init()) {
    Serial.println("SD card init failed!");
    Serial.println("Gallery may not work properly.");
  } else {
    Serial.println("SD card ready.\n");
  }

  // 2. Start the display, touch, and LVGL runtime.
  Serial.println("[2/3] Initializing display...");
  screen.init();

  Serial.printf("LVGL v%d.%d.%d initialized\n\n",
                lv_version_major(),
                lv_version_minor(),
                lv_version_patch());

  // 3. Build the gallery screen and make it active.
  Serial.println("[3/3] Setting up picture gallery UI...");
  setup_scr_picture(&guider_picture_ui);
  lv_scr_load(guider_picture_ui.picture);
  Serial.println();

  Serial.println("========================================");
  Serial.println("  System Ready!");
  Serial.println("========================================");
  Serial.println("Tips:");
  Serial.println("  - Tap left button for previous image");
  Serial.println("  - Tap right button for next image");
  Serial.println("  - Images should be in /picture folder");
  Serial.println("  - Supported format: BMP (240x240)");
  Serial.println("========================================\n");
}

void loop() {
  screen.routine();
  delay(5);
}
