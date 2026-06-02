#include "display.h"
#include "all_in_one_app.h"
#include "RGB_ui.h"

Display screen;

static void run_startup_led_self_test() {
  for (uint8_t i = 0; i < 2; ++i) {
    neopixelWrite(RGB_LED_PIN, 255, 128, 0);
    delay(180);
    neopixelWrite(RGB_LED_PIN, 0, 0, 0);
    delay(80);
  }
}

void setup() {
  // Initialize serial output first so startup diagnostics are visible.
  Serial.begin(115200);
  delay(100);

  Serial.println("\n========================================");
  Serial.println("  ESP32-S3 LVGL All In One");
  Serial.println("========================================\n");

  run_startup_led_self_test();

  screen.init();

  Serial.printf("LVGL v%d.%d.%d initialized\n",
                lv_version_major(),
                lv_version_minor(),
                lv_version_patch());

  // Build the home shell and prepare the shared app controller.
  all_in_one_app_init();
  Serial.println("All-in-One home loaded.\n");
}

void loop() {
  // Let the app controller update active modules, then service LVGL.
  all_in_one_app_loop();
  screen.routine();
  delay(5);
}
