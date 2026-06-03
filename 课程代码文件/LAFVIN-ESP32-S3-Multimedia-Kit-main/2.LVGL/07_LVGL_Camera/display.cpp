#include "display.h"
#include "TFT_eSPI.h"
#include "FT6336U.h"

/***************************************************************************************
 * Screen Configuration
 ***************************************************************************************/
#if TFT_DIRECTION == 0 || TFT_DIRECTION == 2
static const uint16_t screenWidth  = 240;
static const uint16_t screenHeight = 320;
#else
static const uint16_t screenWidth  = 320;
static const uint16_t screenHeight = 240;
#endif

// LVGL draw buffer size (typically set to 1/10 of the screen width)
#define LVGL_BUF_SIZE (screenWidth * 10)

/***************************************************************************************
 * Global Variables
 ***************************************************************************************/
// LVGL draw buffer descriptor
static lv_disp_draw_buf_t draw_buf;
// Actual memory used by the display buffer
static lv_color_t buf[LVGL_BUF_SIZE];

// Hardware driver instances
TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight); /* TFT display instance */
FT6336U ft6336u(I2C_SDA, I2C_SCL, RST_N_PIN, INT_N_PIN); /* Capacitive touch panel instance */

/***************************************************************************************
 * Callback Functions
 ***************************************************************************************/

#if LV_USE_LOG != 0
/**
 * @brief Serial debug print callback
 * Used to redirect LVGL internal logs to Serial.
 */
void my_print(const char * buf)
{
    Serial.printf(buf);
    Serial.flush();
}
#endif

/**
 * @brief Display flush callback
 * Called after LVGL finishes rendering part of the image, and sends the
 * buffer content to the screen.
 *
 * @param disp    Display driver pointer
 * @param area    Area to refresh this time
 * @param color_p Pointer to the color data
 */
void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    // Use startWrite/endWrite to lock the SPI bus for better efficiency.
    tft.startWrite();

    // Set the drawing window and push the color data.
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors((uint16_t*)&color_p->full, w * h, true );

    tft.endWrite();

    // Notify LVGL that flushing is complete.
    lv_disp_flush_ready( disp );
}

/**
 * @brief Touch input read callback
 * Called periodically by LVGL to get the current touch screen state.
 *
 * @param indev_driver Input device driver pointer
 * @param data         Storage for the input data read this time
 */
void my_touchpad_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data )
{
    // Read the touch panel status.
    FT6336U_TouchPointType tp = ft6336u.scan();

    // No touch point detected.
    if( tp.touch_count == 0 )
    {
        data->state = LV_INDEV_STATE_REL; // Released state
    }
    else
    {
        // Get the coordinates of the first touch point.
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
            data->state = LV_INDEV_STATE_PR; // Pressed state
            data->point.x = x;
            data->point.y = y;
        }
        else
        {
            data->state = LV_INDEV_STATE_REL; // Treat invalid coordinates as released
        }
    }
}

/***************************************************************************************
 * Display Class Member Function Implementations
 ***************************************************************************************/

void Display::init(void)
{
    // 1. Initialize the touch controller.
    ft6336u.begin();

    // 2. Register the LVGL log callback, if enabled.
#if LV_USE_LOG != 0
    lv_log_register_print_cb( my_print );
#endif

    // 3. Initialize the LVGL core.
    lv_init();

    // 4. Initialize the TFT display.
    tft.begin();
    tft.setRotation( TFT_DIRECTION ); /* Set the display orientation */
    tft.invertDisplay(0); /* Invert colors if required by the panel characteristics, often needed for IPS panels */

    // 5. Initialize the display buffer.
    lv_disp_draw_buf_init( &draw_buf, buf, NULL, LVGL_BUF_SIZE );

    // 6. Initialize and register the display driver.
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init( &disp_drv );
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush; // Set the flush callback
    disp_drv.draw_buf = &draw_buf;     // Set the draw buffer
    lv_disp_drv_register( &disp_drv );

    // 7. Initialize and register the input device driver (touch panel).
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init( &indev_drv );
    indev_drv.type = LV_INDEV_TYPE_POINTER; // Pointer-type input device
    indev_drv.read_cb = my_touchpad_read;   // Set the read callback
    lv_indev_drv_register( &indev_drv );
}

void Display::routine(void)
{
    // Handle LVGL internal timer tasks, animations, and events.
    lv_task_handler();
}
