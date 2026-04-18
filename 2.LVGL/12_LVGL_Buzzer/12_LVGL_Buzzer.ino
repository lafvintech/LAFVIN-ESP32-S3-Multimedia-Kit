#include "display.h"
#include "Buzzer_ui.h"

Display screen;

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\n========================================");
  Serial.println("  ESP32-S3 LVGL Buzzer");
  Serial.println("========================================\n");

  screen.init();
  Serial.printf("LVGL v%d.%d.%d initialized\n",
                lv_version_major(),
                lv_version_minor(),
                lv_version_patch());

  buzzer_ui_setup(&g_buzzer_ui);
  lv_scr_load(g_buzzer_ui.screen);
  Serial.println("Buzzer screen loaded.\n");
}

void loop() {
  buzzer_ui_loop();
  screen.routine();
  delay(5);
}
