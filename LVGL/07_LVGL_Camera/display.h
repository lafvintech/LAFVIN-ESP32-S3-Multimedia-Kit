#ifndef __DISPLAY_H
#define __DISPLAY_H

#include "lvgl.h"

/***************************************************************************************
 * 硬件引脚配置 (Hardware Pin Configuration)
 * 如果未在其他地方定义，将使用以下默认值
 ***************************************************************************************/

// I2C 引脚定义 (用于触摸屏 FT6336U)
#ifndef I2C_SDA
#define I2C_SDA 2
#endif

#ifndef I2C_SCL
#define I2C_SCL 1
#endif

// 触摸屏复位和中断引脚 (如果不使用设为 -1)
#define RST_N_PIN -1
#define INT_N_PIN -1

// TFT 屏幕显示方向配置
// 0: Portrait
// 1: Landscape (Landscape)
// 2: Portrait Inverted
// 3: Landscape Inverted
#define TFT_DIRECTION 1

/**
 * @class Display
 * @brief 管理 LVGL 初始化、屏幕刷新和触摸输入
 */
class Display
{
private:
    // 这里可以添加私有成员变量，但由于 LVGL 回调通常需要静态函数或全局变量，
    // 目前大部分状态存储在 display.cpp 的静态变量中。

public:
    /**
     * @brief 初始化显示驱动、触摸驱动和 LVGL 库
     */
    void init();

    /**
     * @brief LVGL 任务处理函数
     * @note 需要在 loop() 中循环调用，建议每 5ms 调用一次
     */
    void routine();
};

#endif
