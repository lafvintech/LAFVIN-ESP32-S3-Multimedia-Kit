#include "display.h"
#include "RGB_ui.h"

Display screen;

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n========================================");
  Serial.println("  ESP32-S3 LVGL RGB");
  Serial.println("========================================\n");

  screen.init();
  Serial.printf("LVGL v%d.%d.%d initialized\n",
                lv_version_major(),
                lv_version_minor(),
                lv_version_patch());

  rgb_ui_setup(&g_rgb_ui);
  lv_scr_load(g_rgb_ui.screen);
  Serial.println("RGB screen loaded.\n");
}

void loop() {
  rgb_ui_loop();
  screen.routine();
  delay(5);
}
