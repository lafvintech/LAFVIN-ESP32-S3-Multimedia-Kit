#include "display.h"
#include "TFT_eSPI.h"
#include "FT6336U.h"

/***************************************************************************************
 * 屏幕参数配置 (Screen Configuration)
 ***************************************************************************************/
#if TFT_DIRECTION == 0 || TFT_DIRECTION == 2
static const uint16_t screenWidth  = 240;
static const uint16_t screenHeight = 320;
#else
static const uint16_t screenWidth  = 320;
static const uint16_t screenHeight = 240;
#endif

// LVGL 缓冲区大小设置 (通常设置为屏幕宽度的 1/10)
#define LVGL_BUF_SIZE (screenWidth * 10)

/***************************************************************************************
 * 全局变量定义 (Global Variables)
 ***************************************************************************************/
// LVGL 绘图缓冲区描述符
static lv_disp_draw_buf_t draw_buf;
// 实际的显示缓冲区内存
static lv_color_t buf[LVGL_BUF_SIZE];

// 硬件驱动实例
TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight); /* TFT 显示屏实例 */
FT6336U ft6336u(I2C_SDA, I2C_SCL, RST_N_PIN, INT_N_PIN); /* 电容触摸屏实例 */

/***************************************************************************************
 * 回调函数 (Callback Functions)
 ***************************************************************************************/

#if LV_USE_LOG != 0
/**
 * @brief 串口调试打印回调
 * 用于将 LVGL 的内部日志重定向到 Serial
 */
void my_print(const char * buf)
{
    Serial.printf(buf);
    Serial.flush();
}
#endif

/**
 * @brief 显示刷新回调函数 (Flush Callback)
 * 当 LVGL 完成一部分图像的渲染后，会调用此函数将缓冲区内容发送到屏幕
 * 
 * @param disp    显示驱动指针
 * @param area    本次刷新的区域坐标
 * @param color_p 颜色数据指针
 */
void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    // 使用 TFT_eSPI 的 startWrite/endWrite 锁定 SPI 总线以提高效率
    tft.startWrite();
    
    // 设置绘图窗口并推送颜色数据
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors((uint16_t*)&color_p->full, w * h, true );
    
    tft.endWrite();

    // 通知 LVGL 刷新完成
    lv_disp_flush_ready( disp );
}

/**
 * @brief 触摸输入读取回调函数 (Input Read Callback)
 * LVGL 定期调用此函数以获取触摸屏的状态
 * 
 * @param indev_driver 输入设备驱动指针
 * @param data         用于存储读取到的输入数据
 */
void my_touchpad_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data )
{
    // 读取触摸屏状态
    FT6336U_TouchPointType tp = ft6336u.scan(); 
    
    // 如果没有触摸点
    if( tp.touch_count == 0 )
    {
        data->state = LV_INDEV_STATE_REL; // 释放状态
    }
    else
    {
        // 获取第一个触摸点的坐标
        int x = tp.tp[0].x;
        int y = tp.tp[0].y;

#if TFT_DIRECTION == 1
        int rotated_x = y;
        int rotated_y = 240 - 1 - x;
        x = rotated_x;
        y = rotated_y;
#elif TFT_DIRECTION == 2
        x = 240 - 1 - x;
        y = 320 - 1 - y;
#elif TFT_DIRECTION == 3
        int rotated_x = 320 - 1 - y;
        int rotated_y = x;
        x = rotated_x;
        y = rotated_y;
#endif
        
        // 简单的边界检查，确保坐标在屏幕范围内
        if(x >= 0 && x < screenWidth && y >= 0 && y < screenHeight)
        {
            data->state = LV_INDEV_STATE_PR; // 按下状态
            data->point.x = x;
            data->point.y = y;
        }
        else 
        {
            data->state = LV_INDEV_STATE_REL; // 坐标无效视为释放
        }
    }
}

/***************************************************************************************
 * Display 类成员函数实现
 ***************************************************************************************/

void Display::init(void)
{
    // 1. 初始化触摸驱动
    ft6336u.begin(); 

    // 2. 注册 LVGL 日志回调 (如果启用)
#if LV_USE_LOG != 0
    lv_log_register_print_cb( my_print );
#endif

    // 3. 初始化 LVGL 核心库
    lv_init();

    // 4. 初始化 TFT 屏幕
    tft.begin();          
    tft.setRotation( TFT_DIRECTION ); /* 设置屏幕方向 */
    tft.invertDisplay(0); /* 反转颜色 (根据屏幕面板特性调整，通常 IPS 需要反转) */

    // 5. 初始化显示缓冲区
    lv_disp_draw_buf_init( &draw_buf, buf, NULL, LVGL_BUF_SIZE );

    // 6. 初始化并注册显示驱动
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init( &disp_drv );
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush; // 设置刷新回调
    disp_drv.draw_buf = &draw_buf;     // 设置缓冲区
    lv_disp_drv_register( &disp_drv );

    // 7. 初始化并注册输入设备驱动 (触摸屏)
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init( &indev_drv );
    indev_drv.type = LV_INDEV_TYPE_POINTER; // 指针类型设备
    indev_drv.read_cb = my_touchpad_read;    // 设置读取回调
    lv_indev_drv_register( &indev_drv );
}

void Display::routine(void)
{
    // 处理 LVGL 的内部定时任务、动画和事件
    lv_task_handler();
}
