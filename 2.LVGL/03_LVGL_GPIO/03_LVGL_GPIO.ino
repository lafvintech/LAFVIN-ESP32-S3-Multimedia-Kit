#include "display.h"

// Define the RGB LED pin (WS2812)
#define LED_PIN 48
// Brightness level (0-255)
#define BRIGHTNESS 25

Display screen;

// Label object used to display the current status text
static lv_obj_t * label_status;
static lv_obj_t * main_switch; // Main switch

// Color definition structure
typedef struct {
    const char* name;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    lv_obj_t* checkbox;
} ColorOption;

// Six color options
static ColorOption colors[] = {
    {"Red",     255, 0,   0,   NULL},
    {"Green",   0,   255, 0,   NULL},
    {"Blue",    0,   0,   255, NULL},
    {"Yellow",  255, 255, 0,   NULL},
    {"Cyan",    0,   255, 255, NULL},
    {"Magenta", 255, 0,   255, NULL}
};

static int selected_color = 0; // Index of the currently selected color

/**
 * @brief Helper function: control the LED with brightness scaling
 *
 * @param r Red component (0-255)
 * @param g Green component (0-255)
 * @param b Blue component (0-255)
 */
void ledWrite(uint8_t r, uint8_t g, uint8_t b) {
    // Simple brightness scaling
    uint8_t r_scaled = (r * BRIGHTNESS) / 255;
    uint8_t g_scaled = (g * BRIGHTNESS) / 255;
    uint8_t b_scaled = (b * BRIGHTNESS) / 255;

    // Built-in ESP32 Arduino Core function for controlling the onboard RGB LED
    neopixelWrite(LED_PIN, r_scaled, g_scaled, b_scaled);
}

/**
 * @brief Update the LED output
 */
void updateLED() {
    bool is_on = lv_obj_has_state(main_switch, LV_STATE_CHECKED);

    if(is_on) {
        // Switch is on: show the selected color.
        ledWrite(colors[selected_color].r,
                 colors[selected_color].g,
                 colors[selected_color].b);
        lv_label_set_text_fmt(label_status, "LED: %s", colors[selected_color].name);
    } else {
        // Switch is off: turn off the LED.
        ledWrite(0, 0, 0);
        lv_label_set_text(label_status, "LED: OFF");
    }
}

/**
 * @brief Main switch event callback
 */
static void switch_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        updateLED();
    }
}

/**
 * @brief Checkbox event callback
 */
static void checkbox_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);

    if(code == LV_EVENT_VALUE_CHANGED) {
        // Find the checkbox that was selected.
        for(int i = 0; i < 6; i++) {
            if(colors[i].checkbox == obj) {
                // Clear the checked state of the other checkboxes for single selection.
                for(int j = 0; j < 6; j++) {
                    if(j != i) {
                        lv_obj_clear_state(colors[j].checkbox, LV_STATE_CHECKED);
                    }
                }
                // Update the selected color.
                selected_color = i;
                updateLED();
                break;
            }
        }
    }
}

void setup() {
    Serial.begin(115200);

    // Make sure the LED is off during initialization.
    ledWrite(0, 0, 0);

    // Initialize the display and touch drivers.
    screen.init();

    // 1. Set the background to white.
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_white(), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    // 2. Create the title label.
    lv_obj_t * label_title = lv_label_create(lv_scr_act());
    lv_label_set_text(label_title, "RGB LED Control");
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 10);

    // Set the title style.
    static lv_style_t style_title;
    lv_style_init(&style_title);
    lv_style_set_text_font(&style_title, &lv_font_montserrat_20);
    lv_obj_add_style(label_title, &style_title, 0);

    // 3. Create the main switch widget.
    main_switch = lv_switch_create(lv_scr_act());
    lv_obj_align(main_switch, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_add_event_cb(main_switch, switch_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

    // 4. Create the status label.
    label_status = lv_label_create(lv_scr_act());
    lv_label_set_text(label_status, "LED: OFF");
    lv_obj_align(label_status, LV_ALIGN_TOP_MID, 0, 75);

    // 5. Create six color checkboxes in two columns.
    int start_y = 100;
    int spacing = 35;

    for(int i = 0; i < 6; i++) {
        // Create the checkbox.
        colors[i].checkbox = lv_checkbox_create(lv_scr_act());
        lv_checkbox_set_text(colors[i].checkbox, colors[i].name);

        // Two-column layout: left column (i=0,1,2), right column (i=3,4,5)
        int col = i / 3;  // 0 or 1
        int row = i % 3;  // 0, 1, or 2
        int x_pos = (col == 0) ? 20 : 180;
        int y_pos = start_y + row * spacing;

        lv_obj_set_pos(colors[i].checkbox, x_pos, y_pos);

        // Set the checkbox indicator color.
        lv_obj_set_style_bg_color(colors[i].checkbox,
                                   lv_color_make(colors[i].r, colors[i].g, colors[i].b),
                                   LV_PART_INDICATOR | LV_STATE_CHECKED);

        // Add the event callback.
        lv_obj_add_event_cb(colors[i].checkbox, checkbox_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
    }

    // Select the first color by default (red).
    lv_obj_add_state(colors[0].checkbox, LV_STATE_CHECKED);
}

void loop() {
    // Handle LVGL tasks.
    screen.routine();
    delay(5);
}
