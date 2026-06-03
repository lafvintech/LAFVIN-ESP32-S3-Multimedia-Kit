#include "display.h"
#include "File_Manager_ui.h"

Display screen;

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n========================================");
  Serial.println("  ESP32-S3 LVGL File Manager");
  Serial.println("========================================\n");

  screen.init();
  Serial.printf("LVGL v%d.%d.%d initialized\n",
                lv_version_major(),
                lv_version_minor(),
                lv_version_patch());

  file_manager_ui_setup(&g_file_manager_ui);
  lv_scr_load(g_file_manager_ui.screen);
  Serial.println("File manager loaded.\n");
}

void loop() {
  file_manager_ui_loop();
  screen.routine();
  delay(5);
}
