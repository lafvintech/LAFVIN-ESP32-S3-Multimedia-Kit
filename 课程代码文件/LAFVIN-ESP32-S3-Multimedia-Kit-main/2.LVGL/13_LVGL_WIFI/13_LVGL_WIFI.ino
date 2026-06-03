#include "display.h"
#include "Wifi_ui.h"

Display screen;

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n========================================");
  Serial.println("  ESP32-S3 LVGL WiFi");
  Serial.println("========================================\n");

  screen.init();
  Serial.printf("LVGL v%d.%d.%d initialized\n",
                lv_version_major(),
                lv_version_minor(),
                lv_version_patch());

  wifi_ui_setup(&g_wifi_ui);
  lv_scr_load(g_wifi_ui.screen);
  Serial.println("WiFi screen loaded.\n");
}

void loop() {
  wifi_ui_loop();
  screen.routine();
  delay(5);
}
