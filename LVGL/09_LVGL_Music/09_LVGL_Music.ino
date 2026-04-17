#include "display.h"
#include "music_ui.h"
#include "sd_card.h"

Display screen;

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n========================================");
  Serial.println("  ESP32-S3 LVGL Music");
  Serial.println("========================================\n");

  // Prepare the SD card first because the music UI scans /music on startup.
  Serial.println("[1/3] Initializing SD card...");
  if (!sdcard_init()) {
    Serial.println("SD card init failed!");
    Serial.println("Music UI may not work properly.");
  } else {
    Serial.println("SD card ready.\n");
  }

  // Initialize the LCD, touch driver, and LVGL runtime.
  Serial.println("[2/3] Initializing display...");
  screen.init();
  Serial.printf("LVGL v%d.%d.%d initialized\n\n",
                lv_version_major(),
                lv_version_minor(),
                lv_version_patch());

  // Build the music screen after storage and display are ready.
  Serial.println("[3/3] Setting up music UI...");
  music_ui_setup(&g_music_ui);
  lv_scr_load(g_music_ui.screen);
  Serial.println();
}

void loop() {
  // Keep both the audio pipeline and the LVGL task handler running.
  music_ui_loop();
  screen.routine();
  delay(5);
}
