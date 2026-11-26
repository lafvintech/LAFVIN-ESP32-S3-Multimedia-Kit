#include "display.h"
#include "TFT_eSPI.h"
#include "FT6336U.h"
#include <Wire.h>

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

// LVGL draw buffer size: 10 full-width rows
#define LVGL_BUF_SIZE (screenWidth * 10)

/***************************************************************************************
 * Performance Monitor Switch (can also be set from the build with -D)
 *   1 : measure frame rate / flush time / dirty ratio, print one line per second
 *   0 : fully compiled out (no RAM, no CPU cost)
 ***************************************************************************************/
#ifndef DISPLAY_PERF_MONITOR
#define DISPLAY_PERF_MONITOR    0
#endif

#ifndef PERF_REPORT_PERIOD_MS
#define PERF_REPORT_PERIOD_MS   1000
#endif

/***************************************************************************************
 * Global Variables
 ***************************************************************************************/
// LVGL draw buffer descriptor
static lv_disp_draw_buf_t draw_buf;
// Actual memory used by the display buffer
static lv_color_t buf[LVGL_BUF_SIZE];

// Hardware driver instances
//
// TFT_eSPI takes the native (rotation 0) panel size and setRotation() swaps it for
// landscape. Passing the logical 320x240 here would clip TFT_eSPI's own drawing
// functions (fillScreen, fillRect, ...) at x = 240.
#define PANEL_NATIVE_WIDTH   240
#define PANEL_NATIVE_HEIGHT  320

/***************************************************************************************
 * Deferred display-on
 *
 * GRAM holds random data after power-up, the backlight is always on, and without a
 * TE line every full-screen write tears. So the panel stays off until LVGL's first
 * frame is completely in GRAM:
 *   - tft_init_skip_dispon() stops tft.begin() from turning the panel on (weak hook
 *     in the locally patched TFT_eSPI); init() also sends DISPOFF as a fallback.
 *   - my_disp_flush() sends DISPON after the last band of the first frame.
 * With a stock TFT_eSPI the hook is never called and random GRAM is visible for
 * ~120 ms inside begin(); the first frame is still hidden.
 *
 * Requirement: lv_scr_load() the first screen before the first lv_task_handler(),
 * otherwise the panel turns on showing the empty default screen.
 * Set DISPLAY_DEFER_ON to 0 to restore the stock behaviour.
 ***************************************************************************************/
#ifndef DISPLAY_DEFER_ON
#define DISPLAY_DEFER_ON        1
#endif

#if DISPLAY_DEFER_ON
static bool s_panel_on = false;
#endif

/**
 * @brief Overrides TFT_eSPI's weak display-on hook (patched library only).
 * @return true to skip DISPON inside tft.begin(); my_disp_flush() turns the
 *         panel on after the first complete frame instead.
 */
bool tft_init_skip_dispon( void )
{
    return DISPLAY_DEFER_ON;
}

TFT_eSPI tft = TFT_eSPI(PANEL_NATIVE_WIDTH, PANEL_NATIVE_HEIGHT); /* TFT display instance */
FT6336U ft6336u(I2C_SDA, I2C_SCL, RST_N_PIN, INT_N_PIN); /* Capacitive touch panel instance */

/***************************************************************************************
 * Performance Counters
 *
 *   frame : CPU time of one refresh cycle (layout + render + SPI flush), reported
 *           by LVGL's monitor_cb. Compare it with (1000 / fps): if frame is much
 *           smaller than the frame interval, the CPU has headroom and the limiter
 *           is LV_DISP_DEF_REFR_PERIOD, not compute.
 *   flush : pure SPI transfer time inside my_disp_flush(). If frame >> flush, the
 *           bottleneck is rendering (too many redraws / shadows / rounded corners)
 *           and not the SPI bus.
 *   task  : how long lv_task_handler() blocks the main loop. If task approaches the
 *           loop period, the UI is CPU-bound and the main loop is starving.
 *   touch : time spent reading the touch controller over I2C. It runs inside
 *           lv_task_handler() but outside the refresh cycle, so it inflates task
 *           without inflating frame.
 *   loop  : the real main-loop period (span / number of routine() calls), includes
 *           the delay(5) in loop().
 *   calls/frame : how many main-loop iterations produce one frame. Near 1.0 means the
 *           loop is frame-bound; well above 1.0 means most iterations have nothing to
 *           draw and LVGL is idling.
 *   dirty : redrawn pixels / full screen pixels. A button press should only dirty
 *           a few percent. A value near 100% means something redraws the whole screen
 *           every frame, and that is usually the real problem.
 ***************************************************************************************/
#if DISPLAY_PERF_MONITOR

typedef struct
{
    uint32_t window_start;   // start of the current measurement window (millis)
    uint32_t frames;         // monitor_cb calls == real refresh cycles
    uint32_t px_total;       // pixels flushed in this window
    uint32_t frame_ms_sum;   // accumulated whole-cycle time
    uint32_t frame_ms_max;
    uint32_t flush_us_sum;   // accumulated SPI transfer time
    uint32_t flush_us_max;
    uint32_t flush_calls;
    uint32_t task_us_sum;    // accumulated lv_task_handler() time
    uint32_t task_us_max;
    uint32_t task_calls;
    uint32_t touch_us_sum;   // accumulated touch read time (I2C)
    uint32_t touch_us_max;
    uint32_t touch_calls;
} PerfStats;

static PerfStats s_perf;

static void perf_report( void )
{
    uint32_t now  = millis();
    uint32_t span = now - s_perf.window_start;

    if( span < PERF_REPORT_PERIOD_MS )
    {
        return;
    }

    float span_s    = span / 1000.0f;
    float loop_ms   = s_perf.task_calls  ? (float)span / s_perf.task_calls : 0.0f;
    float fps       = s_perf.frames / span_s;
    float frame_ms  = s_perf.frames      ? (float)s_perf.frame_ms_sum / s_perf.frames : 0.0f;
    float flush_ms  = s_perf.flush_calls ? (s_perf.flush_us_sum / 1000.0f) / s_perf.flush_calls : 0.0f;
    float task_ms   = s_perf.task_calls  ? (s_perf.task_us_sum  / 1000.0f) / s_perf.task_calls  : 0.0f;
    float touch_ms  = s_perf.touch_calls ? (s_perf.touch_us_sum / 1000.0f) / s_perf.touch_calls : 0.0f;
    float cpf       = s_perf.frames      ? (float)s_perf.task_calls / s_perf.frames : 0.0f;
    float px_s      = s_perf.px_total / span_s;
    float kbps      = px_s * 2.0f / 1024.0f;
    float screen_px = (float)screenWidth * (float)screenHeight;
    float dirty     = ( fps > 0.01f ) ? ( px_s / ( fps * screen_px ) ) * 100.0f : 0.0f;

    Serial.printf(
        "[perf] fps=%5.1f loop=%5.1fms calls/frame=%4.1f | task=%5.1fms touch=%4.2fms(max %4.2f) "
        "| frame=%5.1fms(max %2ums) flush=%4.2fms | dirty=%5.1f%% %6.0fpx/s ~%6.1fKB/s\n",
        fps,
        loop_ms, cpf,
        task_ms, touch_ms, s_perf.touch_us_max / 1000.0f,
        frame_ms, s_perf.frame_ms_max, flush_ms,
        dirty, px_s, kbps );

    s_perf = PerfStats();
    s_perf.window_start = now;
}

/**
 * @brief LVGL refresh monitor callback
 * Invoked by the LVGL refresh timer once per frame that actually flushed
 * something. `time_ms` covers layout + render + flush, `px` is the number of
 * pixels sent to the panel in that cycle.
 *
 * Deliberately does NOT print here: Serial output blocks for milliseconds and
 * would distort exactly the numbers we are trying to measure.
 */
static void my_monitor_cb( lv_disp_drv_t * disp_drv, uint32_t time_ms, uint32_t px )
{
    (void)disp_drv;

    s_perf.frames++;
    s_perf.px_total     += px;
    s_perf.frame_ms_sum += time_ms;

    if( time_ms > s_perf.frame_ms_max )
    {
        s_perf.frame_ms_max = time_ms;
    }
}

#endif /* DISPLAY_PERF_MONITOR */

/***************************************************************************************
 * Fast touch read
 *
 * The FT6336U library's readByte() waits delay(10) after every byte and scan() does
 * 1 to 7 reads, i.e. 10-70 ms of busy-wait per poll. Everything needed lives in
 * registers 0x02..0x06, so one 5-byte burst read at 400 kHz (~0.2 ms) replaces it.
 *
 * Set USE_STOCK_TOUCH_SCAN to 1 to fall back to the library implementation.
 ***************************************************************************************/
#define USE_STOCK_TOUCH_SCAN   0

#define FT6336U_I2C_ADDR       0x38
#define FT6336U_REG_TD_STATUS  0x02

/**
 * @brief Burst-read touch status and touch point 1 coordinates.
 *
 * @param count Receives TD_STATUS low nibble (number of touch points).
 * @param x     Receives touch point 1 X in panel coordinates.
 * @param y     Receives touch point 1 Y in panel coordinates.
 * @return true on success, false if the I2C transfer failed.
 */
static bool fast_touch_read( uint8_t * count, int * x, int * y )
{
    Wire.beginTransmission( FT6336U_I2C_ADDR );
    Wire.write( FT6336U_REG_TD_STATUS );

    if( Wire.endTransmission( false ) != 0 )   // repeated start, no STOP
    {
        return false;
    }

    if( Wire.requestFrom( FT6336U_I2C_ADDR, (size_t)5 ) != 5 )
    {
        return false;
    }

    uint8_t td = Wire.read();   // 0x02 TD_STATUS
    uint8_t xh = Wire.read();   // 0x03 TOUCH1_XH : event[7:6] id[5:4] x[11:8][3:0]
    uint8_t xl = Wire.read();   // 0x04 TOUCH1_XL : x[7:0]
    uint8_t yh = Wire.read();   // 0x05 TOUCH1_YH : event[7:6] id[5:4] y[11:8][3:0]
    uint8_t yl = Wire.read();   // 0x06 TOUCH1_YL : y[7:0]

    *count = td & 0x0F;
    *x     = ( ( xh & 0x0F ) << 8 ) | xl;
    *y     = ( ( yh & 0x0F ) << 8 ) | yl;

    return true;
}

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
    Serial.print( buf );
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

#if DISPLAY_PERF_MONITOR
    uint32_t t_begin = micros();
#endif

    // Use startWrite/endWrite to lock the SPI bus for better efficiency.
    tft.startWrite();

    // Set the drawing window and push the color data.
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors((uint16_t*)&color_p->full, w * h, true );

    tft.endWrite();

#if DISPLAY_DEFER_ON
    // The last band of the first frame is in GRAM: the whole screen now holds a
    // valid image, so it is safe to show it.
    if( !s_panel_on && lv_disp_flush_is_last( disp ) )
    {
        tft.writecommand( (uint8_t)TFT_DISPON );
        s_panel_on = true;
    }
#endif

#if DISPLAY_PERF_MONITOR
    uint32_t t_cost = micros() - t_begin;
    s_perf.flush_us_sum += t_cost;
    s_perf.flush_calls++;

    if( t_cost > s_perf.flush_us_max )
    {
        s_perf.flush_us_max = t_cost;
    }
#endif

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
    (void)indev_driver;

#if DISPLAY_PERF_MONITOR
    uint32_t t_begin = micros();
#endif

#if USE_STOCK_TOUCH_SCAN
    FT6336U_TouchPointType tp = ft6336u.scan();
    uint8_t touch_count = tp.touch_count;
    int x = tp.tp[0].x;
    int y = tp.tp[0].y;
#else
    uint8_t touch_count = 0;
    int x = 0;
    int y = 0;

    if( !fast_touch_read( &touch_count, &x, &y ) )
    {
        touch_count = 0; // I2C error, treat as released
    }
#endif

#if DISPLAY_PERF_MONITOR
    uint32_t t_cost = micros() - t_begin;
    s_perf.touch_us_sum += t_cost;
    s_perf.touch_calls++;

    if( t_cost > s_perf.touch_us_max )
    {
        s_perf.touch_us_max = t_cost;
    }
#endif

    // No touch point detected.
    if( touch_count == 0 )
    {
        data->state = LV_INDEV_STATE_REL; // Released state
    }
    else
    {

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

        // Basic boundary check to ensure the coordinates are on screen.
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

    // 400 kHz for fast_touch_read(); the library leaves the bus at 100 kHz.
    Wire.setClock( 400000 );

    // 2. Register the LVGL log callback, if enabled.
#if LV_USE_LOG != 0
    lv_log_register_print_cb( my_print );
#endif

    // 3. Initialize the LVGL core.
    lv_init();

    // 4. Initialize the TFT display.
    tft.begin();
#if DISPLAY_DEFER_ON
    // Keep the panel dark until the first LVGL frame is complete (see my_disp_flush).
    // Only matters with a stock TFT_eSPI; the patched one never turned it on.
    tft.writecommand( (uint8_t)TFT_DISPOFF );
#endif
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

#if DISPLAY_PERF_MONITOR
    disp_drv.monitor_cb = my_monitor_cb; // collect per-frame statistics
    s_perf = PerfStats();
    s_perf.window_start = millis();
#endif

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
#if DISPLAY_PERF_MONITOR
    uint32_t t_begin = micros();
#endif

    // Handle LVGL internal timer tasks, animations, and events.
    lv_task_handler();

#if DISPLAY_PERF_MONITOR
    uint32_t t_cost = micros() - t_begin;
    s_perf.task_us_sum += t_cost;
    s_perf.task_calls++;

    if( t_cost > s_perf.task_us_max )
    {
        s_perf.task_us_max = t_cost;
    }

    perf_report();
#endif
}
