#ifndef __DISPLAY_H
#define __DISPLAY_H

#include "lvgl.h"

/***************************************************************************************
 * Hardware Pin Configuration
 * The following default values will be used if they are not defined elsewhere.
 ***************************************************************************************/

// I2C pin definitions (used for the FT6336U touch controller)
#ifndef I2C_SDA
#define I2C_SDA 2
#endif

#ifndef I2C_SCL
#define I2C_SCL 1
#endif

// Touch panel reset and interrupt pins (-1 means not used)
#define RST_N_PIN -1
#define INT_N_PIN -1

// TFT display orientation configuration
// 0: Portrait
// 1: Landscape
// 2: Portrait Inverted
// 3: Landscape Inverted
#define TFT_DIRECTION 1

/**
 * @class Display
 * @brief Manages LVGL initialization, screen refresh, and touch input.
 */
class Display
{
private:
    // Private members can be added here. In this project, most runtime state
    // is currently stored in static variables inside display.cpp because LVGL
    // callbacks usually rely on static functions or global/static objects.
public:
    /**
     * @brief Initializes the display driver, touch driver, and LVGL core.
     */
    void init();

    /**
     * @brief LVGL task processing function.
     * @note Call this repeatedly in loop(), preferably once every 5 ms.
     */
    void routine();
};

#endif
