#include "display.h"
#include "hartrate_ui.h"

Display screen;

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n========================================");
  Serial.println("  ESP32-S3 LVGL HeartRate");
  Serial.println("========================================\n");

  screen.init();
  Serial.printf("LVGL v%d.%d.%d initialized\n",
                lv_version_major(),
                lv_version_minor(),
                lv_version_patch());

  hartrate_ui_setup(&g_heartrate_ui);
  lv_scr_load(g_heartrate_ui.screen);
  Serial.println("Heart rate screen loaded.\n");
}

void loop() {
  hartrate_ui_loop();
  screen.routine();
  delay(5);
}
