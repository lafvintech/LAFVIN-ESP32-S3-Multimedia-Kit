#ifndef IMAGE_DATA_H
#define IMAGE_DATA_H

#include <lvgl.h>

// 声明蜂鸣器图标（定义在 buzzer.c 中）
// 这是一个 200x200 的图标，使用 LVGL Image Converter 生成
// 在线工具：https://lvgl.io/tools/imageconverter

// 外部声明：蜂鸣器图标（开启状态）
extern const lv_img_dsc_t buzzer;

// 外部声明：静音图标（关闭状态）
extern const lv_img_dsc_t mute;

#endif // IMAGE_DATA_H
