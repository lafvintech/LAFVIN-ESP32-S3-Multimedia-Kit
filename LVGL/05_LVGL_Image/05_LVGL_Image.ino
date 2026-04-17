#include "display.h"
#include "image_data.h"

Display screen;

void setup() {
    Serial.begin(115200);
    
    // 初始化显示驱动
    screen.init();

    // 设置背景为白色
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_white(), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    // 创建标题
    lv_obj_t * label_title = lv_label_create(lv_scr_act());
    lv_label_set_text(label_title, "Embedded Image Demo");
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 10);
    
    // 标题样式
    static lv_style_t style_title;
    lv_style_init(&style_title);
    lv_style_set_text_font(&style_title, &lv_font_montserrat_20);
    lv_obj_add_style(label_title, &style_title, 0);
    
    Serial.println("Mode: Embedded Image (C Array)");
    
    // 创建图片对象
    lv_obj_t * img = lv_img_create(lv_scr_act());
    lv_img_set_src(img, &StarryNight);  // 设置图片源为嵌入式数据 (240x320 StarryNight 图片)
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);  // 居中显示
    
    // 添加说明文字
    lv_obj_t * label_info = lv_label_create(lv_scr_act());
    lv_label_set_text(label_info, "StarryNight Image\n240x320 from C array");
    lv_obj_align(label_info, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_text_align(label_info, LV_TEXT_ALIGN_CENTER, 0);
    
    Serial.println("✓ Image displayed successfully!");
}

void loop() {
    screen.routine();
    delay(5);
}
