#include "display.h"
#include "image_data.h"

// 蜂鸣器引脚定义
#define BUZZER_PIN 45

// 蜂鸣器类型选择
// true = 有源蜂鸣器（Active Buzzer）- 直接 HIGH/LOW 控制
// false = 无源蜂鸣器（Passive Buzzer）- 需要 PWM 方波
#define ACTIVE_BUZZER false  // 改为 false 如果是无源蜂鸣器

// 无源蜂鸣器参数（仅在 ACTIVE_BUZZER = false 时使用）
#define BUZZER_FREQ 2000    // 频率 2kHz
#define BUZZER_CHANNEL 0    // PWM 通道
#define BUZZER_RESOLUTION 8 // PWM 分辨率 8-bit

Display screen;

// 全局变量
static bool buzzer_on = false;
static lv_obj_t * img_btn_buzzer;  // 蜂鸣器按钮（左边）
static lv_obj_t * img_btn_mute;    // 静音按钮（右边）

/**
 * @brief 控制蜂鸣器
 */
void setBuzzer(bool state) {
    buzzer_on = state;
    
    if(ACTIVE_BUZZER) {
        // 有源蜂鸣器：直接 HIGH/LOW
        digitalWrite(BUZZER_PIN, state ? HIGH : LOW);
    } else {
        // 无源蜂鸣器：使用 PWM
        if(state) {
            ledcWriteTone(BUZZER_PIN, BUZZER_FREQ);  // 输出频率
        } else {
            ledcWriteTone(BUZZER_PIN, 0);  // 停止输出
        }
    }
    
}

/**
 * @brief 蜂鸣器按钮事件回调（左边）
 */
static void buzzer_btn_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        // 开启蜂鸣器
        setBuzzer(true);
        Serial.println("Buzzer button clicked: ON");
    }
}

/**
 * @brief 静音按钮事件回调（右边）
 */
static void mute_btn_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        // 关闭蜂鸣器
        setBuzzer(false);
        Serial.println("Mute button clicked: OFF");
    }
}

void setup() {
    Serial.begin(115200);
    
    // 初始化蜂鸣器
    if(ACTIVE_BUZZER) {
        // 有源蜂鸣器：普通 GPIO
        pinMode(BUZZER_PIN, OUTPUT);
        digitalWrite(BUZZER_PIN, LOW);
        Serial.println("Buzzer Type: Active (有源)");
    } else {
        // 无源蜂鸣器：PWM
        ledcAttach(BUZZER_PIN, BUZZER_FREQ, BUZZER_RESOLUTION);
        ledcWriteTone(BUZZER_PIN, 0);  // 初始关闭
        Serial.println("Buzzer Type: Passive (无源)");
        Serial.printf("Frequency: %d Hz\n", BUZZER_FREQ);
    }
    
    Serial.println("\n========================================");
    Serial.println("06_LVGL_IMG_BTN - Image Button Demo");
    Serial.println("========================================\n");
    
    // 初始化显示驱动
    screen.init();

    // 设置背景为白色
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_white(), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    // 创建标题
    lv_obj_t * label_title = lv_label_create(lv_scr_act());
    lv_label_set_text(label_title, "Buzzer Control");
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 10);
    
    // 标题样式
    static lv_style_t style_title;
    lv_style_init(&style_title);
    lv_style_set_text_font(&style_title, &lv_font_montserrat_20);
    lv_style_set_text_color(&style_title, lv_color_black());
    lv_obj_add_style(label_title, &style_title, 0);

    // 创建左边的蜂鸣器按钮（使用 lv_img 代替 lv_imgbtn）
    img_btn_buzzer = lv_img_create(lv_scr_act());
    lv_img_set_src(img_btn_buzzer, &buzzer);
    lv_obj_align(img_btn_buzzer, LV_ALIGN_CENTER, -60, 0);  // 左边偏移 -60
    lv_obj_add_flag(img_btn_buzzer, LV_OBJ_FLAG_CLICKABLE);  // 使图片可点击
    lv_obj_add_event_cb(img_btn_buzzer, buzzer_btn_event_handler, LV_EVENT_CLICKED, NULL);
    
    // 创建右边的静音按钮（使用 lv_img 代替 lv_imgbtn）
    img_btn_mute = lv_img_create(lv_scr_act());
    lv_img_set_src(img_btn_mute, &mute);
    lv_obj_align(img_btn_mute, LV_ALIGN_CENTER, 60, 0);  // 右边偏移 +60
    lv_obj_add_flag(img_btn_mute, LV_OBJ_FLAG_CLICKABLE);  // 使图片可点击
    lv_obj_add_event_cb(img_btn_mute, mute_btn_event_handler, LV_EVENT_CLICKED, NULL);
    

    // 添加左边按钮的标签
    lv_obj_t * label_buzzer = lv_label_create(lv_scr_act());
    lv_label_set_text(label_buzzer, "ON");
    lv_obj_align(label_buzzer, LV_ALIGN_CENTER, -60, 110);
    lv_obj_set_style_text_color(label_buzzer, lv_color_make(0, 150, 0), 0);
    
    // 添加右边按钮的标签
    lv_obj_t * label_mute_btn = lv_label_create(lv_scr_act());
    lv_label_set_text(label_mute_btn, "OFF");
    lv_obj_align(label_mute_btn, LV_ALIGN_CENTER, 60, 110);
    lv_obj_set_style_text_color(label_mute_btn, lv_color_make(150, 0, 0), 0);
    
    Serial.println("✓ Setup complete!");
    Serial.println("Left button: Turn ON buzzer");
    Serial.println("Right button: Turn OFF buzzer\n");
}

void loop() {
    screen.routine();
    delay(5);
}
