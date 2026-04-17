#include "display.h"

// 定义 RGB LED 引脚 (WS2812)
#define LED_PIN 48
// 亮度级别 (0-255)
#define BRIGHTNESS 25 

Display screen;

// 标签对象，用于显示当前状态文字
static lv_obj_t * label_status;
static lv_obj_t * main_switch; // 主开关

// 颜色定义结构体
typedef struct {
    const char* name;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    lv_obj_t* checkbox;
} ColorOption;

// 6 种颜色选项
static ColorOption colors[] = {
    {"Red",     255, 0,   0,   NULL},
    {"Green",   0,   255, 0,   NULL},
    {"Blue",    0,   0,   255, NULL},
    {"Yellow",  255, 255, 0,   NULL},
    {"Cyan",    0,   255, 255, NULL},
    {"Magenta", 255, 0,   255, NULL}
};

static int selected_color = 0; // 当前选中的颜色索引

/**
 * @brief 辅助函数：带亮度缩放的 LED 控制
 * 
 * @param r 红色分量 (0-255)
 * @param g 绿色分量 (0-255)
 * @param b 蓝色分量 (0-255)
 */
void ledWrite(uint8_t r, uint8_t g, uint8_t b) {
    // 简单的亮度缩放
    uint8_t r_scaled = (r * BRIGHTNESS) / 255;
    uint8_t g_scaled = (g * BRIGHTNESS) / 255;
    uint8_t b_scaled = (b * BRIGHTNESS) / 255;
    
    // ESP32 Arduino Core 内置函数，用于控制板载 RGB LED
    neopixelWrite(LED_PIN, r_scaled, g_scaled, b_scaled);
}

/**
 * @brief 更新 LED 显示
 */
void updateLED() {
    bool is_on = lv_obj_has_state(main_switch, LV_STATE_CHECKED);
    
    if(is_on) {
        // 开关打开 -> 显示选中的颜色
        ledWrite(colors[selected_color].r, 
                 colors[selected_color].g, 
                 colors[selected_color].b);
        lv_label_set_text_fmt(label_status, "LED: %s", colors[selected_color].name);
    } else {
        // 开关关闭 -> 熄灭 LED
        ledWrite(0, 0, 0);
        lv_label_set_text(label_status, "LED: OFF");
    }
}

/**
 * @brief 主开关事件回调函数
 */
static void switch_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_VALUE_CHANGED) {
        updateLED();
    }
}

/**
 * @brief Checkbox 事件回调函数
 */
static void checkbox_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    
    if(code == LV_EVENT_VALUE_CHANGED) {
        // 找到被勾选的 checkbox
        for(int i = 0; i < 6; i++) {
            if(colors[i].checkbox == obj) {
                // 取消其他 checkbox 的勾选状态（单选效果）
                for(int j = 0; j < 6; j++) {
                    if(j != i) {
                        lv_obj_clear_state(colors[j].checkbox, LV_STATE_CHECKED);
                    }
                }
                // 更新选中的颜色
                selected_color = i;
                updateLED();
                break;
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    
    // 初始化时确保 LED 关闭
    ledWrite(0, 0, 0);

    // 初始化显示和触摸驱动
    screen.init();

    // 1. 设置背景为白色
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_white(), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    // 2. 创建标题标签
    lv_obj_t * label_title = lv_label_create(lv_scr_act());
    lv_label_set_text(label_title, "RGB LED Control");
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 10);
    
    // 设置标题样式
    static lv_style_t style_title;
    lv_style_init(&style_title);
    lv_style_set_text_font(&style_title, &lv_font_montserrat_20);
    lv_obj_add_style(label_title, &style_title, 0);

    // 3. 创建主开关 (Switch) 控件
    main_switch = lv_switch_create(lv_scr_act());
    lv_obj_align(main_switch, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_add_event_cb(main_switch, switch_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

    // 4. 创建状态显示标签
    label_status = lv_label_create(lv_scr_act());
    lv_label_set_text(label_status, "LED: OFF");
    lv_obj_align(label_status, LV_ALIGN_TOP_MID, 0, 75);

    // 5. 创建 6 个颜色 Checkbox（分两列显示）
    int start_y = 100;
    int spacing = 35;
    
    for(int i = 0; i < 6; i++) {
        // 创建 checkbox
        colors[i].checkbox = lv_checkbox_create(lv_scr_act());
        lv_checkbox_set_text(colors[i].checkbox, colors[i].name);
        
        // 分两列布局：左列 (i=0,1,2)，右列 (i=3,4,5)
        int col = i / 3;  // 0 或 1
        int row = i % 3;  // 0, 1, 或 2
        int x_offset = (col == 0) ? -60 : 60;
        int y_offset = start_y + row * spacing;
        
        lv_obj_align(colors[i].checkbox, LV_ALIGN_TOP_MID, x_offset, y_offset);
        
        // 设置 checkbox 颜色
        lv_obj_set_style_bg_color(colors[i].checkbox, 
                                   lv_color_make(colors[i].r, colors[i].g, colors[i].b), 
                                   LV_PART_INDICATOR | LV_STATE_CHECKED);
        
        // 添加事件回调
        lv_obj_add_event_cb(colors[i].checkbox, checkbox_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    }
    
    // 默认选中第一个颜色（红色）
    lv_obj_add_state(colors[0].checkbox, LV_STATE_CHECKED);
}

void loop() {
    // 处理 LVGL 任务
    screen.routine();
    delay(5);
}
