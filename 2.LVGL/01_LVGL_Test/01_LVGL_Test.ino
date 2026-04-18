#include "display.h"

Display screen;

void setup() {
    Serial.begin(115200);
    
    // Initialize the display and touch driver using the class method
    screen.init();

    // 设置背景为白色
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_white(), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
    lv_obj_invalidate(lv_scr_act());
    // Create a simple label
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "TEST\nHello World!\nArduino LVGL");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    
    // Optional: style the label to be bigger and centered text
    static lv_style_t style;
    lv_style_init(&style);
    // Make sure montserrat 20 is enabled in lv_conf.h, otherwise use default or smaller
    lv_style_set_text_font(&style, &lv_font_montserrat_20); 
    lv_style_set_text_align(&style, LV_TEXT_ALIGN_CENTER);
    lv_obj_add_style(label, &style, 0);
}

void loop() {
    // Handle LVGL tasks
    screen.routine();
    delay(5);
}

