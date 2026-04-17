#include "display.h"

Display screen;

// 全局对象变量
static lv_obj_t * obj_ball;     // 小球对象
static lv_obj_t * slider_r;     // 红色分量滑块
static lv_obj_t * slider_g;     // 绿色分量滑块
static lv_obj_t * slider_b;     // 蓝色分量滑块

static lv_obj_t * label_val_r;  // 显示 R 值的标签
static lv_obj_t * label_val_g;  // 显示 G 值的标签
static lv_obj_t * label_val_b;  // 显示 B 值的标签

/**
 * @brief 滑块事件回调函数
 * 当任意一个滑块的值发生变化时，都会触发此函数
 */
static void slider_event_cb(lv_event_t * e)
{
    // 1. 读取三个滑块的当前值 (0-255)
    // 虽然事件是由某个特定的滑块触发的，但我们需要所有三个值来合成颜色
    // 所以直接读取全局变量是最简单的方法
    int r = lv_slider_get_value(slider_r);
    int g = lv_slider_get_value(slider_g);
    int b = lv_slider_get_value(slider_b);

    // 2. 更新小球的背景颜色
    // lv_color_make(r, g, b) 会根据当前系统的颜色深度（通常是 16bit 565）自动转换
    lv_obj_set_style_bg_color(obj_ball, lv_color_make(r, g, b), 0);

    // 3. 更新滑块旁边的数值标签
    lv_label_set_text_fmt(label_val_r, "%d", r);
    lv_label_set_text_fmt(label_val_g, "%d", g);
    lv_label_set_text_fmt(label_val_b, "%d", b);
}

/**
 * @brief 辅助函数：创建一个带标签的滑块
 * 
 * @param color 滑块的强调色 (Red/Green/Blue)
 * @param y_pos Y轴位置
 * @param slider_ptr 指向滑块全局指针的指针 (输出)
 * @param label_ptr 指向标签全局指针的指针 (输出)
 */
void create_color_slider(lv_color_t color, int y_pos, lv_obj_t ** slider_ptr, lv_obj_t ** label_ptr)
{
    // 1. 创建滑块
    lv_obj_t * slider = lv_slider_create(lv_scr_act());
    lv_obj_set_size(slider, 180, 20);                // 宽 180, 高 20
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, y_pos); // 居中，Y轴偏移
    lv_slider_set_range(slider, 0, 255);             // 设置范围 0-255
    lv_slider_set_value(slider, 255, LV_ANIM_OFF);   // 默认最大值 (白色)
    
    // 设置滑块指示器(Indicator)和旋钮(Knob)的颜色
    // Part Main: 背景槽; Part Indicator: 已滑动部分; Part Knob: 旋钮
    lv_obj_set_style_bg_color(slider, color, LV_PART_INDICATOR); 
    lv_obj_set_style_bg_color(slider, color, LV_PART_KNOB);

    // 添加事件回调
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    *slider_ptr = slider; // 将创建的对象赋值给全局指针

    // 2. 创建数值显示标签 (在滑块右侧)
    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "255");
    // 将标签放在滑块右边 10px 处
    lv_obj_align_to(label, slider, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    
    *label_ptr = label; // 将创建的对象赋值给全局指针
}

void setup() {
    Serial.begin(115200);
    
    // 初始化显示
    screen.init();

    // 设置背景为浅灰色，以便看清白色小球
    lv_obj_set_style_bg_color(lv_scr_act(), lv_palette_lighten(LV_PALETTE_GREY, 4), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    // -------------------------------------------------
    // 1. 创建小球 (The Ball)
    // -------------------------------------------------
    obj_ball = lv_obj_create(lv_scr_act());
    lv_obj_set_size(obj_ball, 80, 80);          // 设置大小 80x80
    lv_obj_set_style_radius(obj_ball, LV_RADIUS_CIRCLE, 0); // 设置圆角为最大（圆形）
    lv_obj_align(obj_ball, LV_ALIGN_TOP_MID, 0, 30);        // 屏幕上方居中
    
    // 初始颜色：白色 (R255 G255 B255)
    lv_obj_set_style_bg_color(obj_ball, lv_color_white(), 0);

    // 添加 "Ball" 标题
    lv_obj_t * label_title = lv_label_create(lv_scr_act());
    lv_label_set_text(label_title, "RGB Color Mixer");
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 5);

    // -------------------------------------------------
    // 2. 创建三个滑块 (Sliders)
    // -------------------------------------------------
    // Red Slider (Y轴偏移 20) - 第一个位置
    create_color_slider(lv_palette_main(LV_PALETTE_RED), 20, &slider_r, &label_val_r);

    // Green Slider (Y轴偏移 60) - 第二个位置
    create_color_slider(lv_palette_main(LV_PALETTE_GREEN), 60, &slider_g, &label_val_g);

    // Blue Slider (Y轴偏移 100) - 第三个位置
    create_color_slider(lv_palette_main(LV_PALETTE_BLUE), 100, &slider_b, &label_val_b);

    // 额外添加 R/G/B 字母标签在滑块左侧
    lv_obj_t* l_r = lv_label_create(lv_scr_act()); lv_label_set_text(l_r, "R"); lv_obj_align_to(l_r, slider_r, LV_ALIGN_OUT_LEFT_MID, -10, 0);
    lv_obj_t* l_g = lv_label_create(lv_scr_act()); lv_label_set_text(l_g, "G"); lv_obj_align_to(l_g, slider_g, LV_ALIGN_OUT_LEFT_MID, -10, 0);
    lv_obj_t* l_b = lv_label_create(lv_scr_act()); lv_label_set_text(l_b, "B"); lv_obj_align_to(l_b, slider_b, LV_ALIGN_OUT_LEFT_MID, -10, 0);
}

void loop() {
    screen.routine();
    delay(5);
}
