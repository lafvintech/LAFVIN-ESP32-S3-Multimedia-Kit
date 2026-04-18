#include "display.h"

Display screen;

// Global object variables
static lv_obj_t * obj_ball;     // Ball object
static lv_obj_t * slider_r;     // Red component slider
static lv_obj_t * slider_g;     // Green component slider
static lv_obj_t * slider_b;     // Blue component slider

static lv_obj_t * label_val_r;  // Label showing the R value
static lv_obj_t * label_val_g;  // Label showing the G value
static lv_obj_t * label_val_b;  // Label showing the B value

/**
 * @brief Slider event callback
 * Called whenever the value of any slider changes.
 */
static void slider_event_cb(lv_event_t * e)
{
    // 1. Read the current values of all three sliders (0-255).
    // Even though the event is triggered by a specific slider, we need
    // all three values to compose the current color.
    int r = lv_slider_get_value(slider_r);
    int g = lv_slider_get_value(slider_g);
    int b = lv_slider_get_value(slider_b);

    // 2. Update the ball background color.
    // lv_color_make(r, g, b) converts the values to the current system color depth.
    lv_obj_set_style_bg_color(obj_ball, lv_color_make(r, g, b), 0);

    // 3. Update the numeric labels beside the sliders.
    lv_label_set_text_fmt(label_val_r, "%d", r);
    lv_label_set_text_fmt(label_val_g, "%d", g);
    lv_label_set_text_fmt(label_val_b, "%d", b);
}

/**
 * @brief Helper function: create a slider with its value label
 *
 * @param color Slider highlight color (Red/Green/Blue)
 * @param y_pos Y-axis position
 * @param slider_ptr Pointer to the global slider pointer (output)
 * @param label_ptr Pointer to the global label pointer (output)
 */
void create_color_slider(lv_color_t color, int y_pos, lv_obj_t ** slider_ptr, lv_obj_t ** label_ptr)
{
    // 1. Create the slider.
    lv_obj_t * slider = lv_slider_create(lv_scr_act());
    lv_obj_set_size(slider, 180, 20);                // Width 180, height 20
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, y_pos); // Centered with Y offset
    lv_slider_set_range(slider, 0, 255);             // Range 0-255
    lv_slider_set_value(slider, 255, LV_ANIM_OFF);   // Default to maximum (white)

    // Set the colors of the slider indicator and knob.
    // Part Main: background track
    // Part Indicator: filled portion
    // Part Knob: handle
    lv_obj_set_style_bg_color(slider, color, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, color, LV_PART_KNOB);

    // Add the event callback.
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    *slider_ptr = slider; // Store the created object in the global pointer

    // 2. Create the value label to the right of the slider.
    lv_obj_t * label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "255");
    lv_obj_align_to(label, slider, LV_ALIGN_OUT_RIGHT_MID, 10, 0); // Place it 10px to the right

    *label_ptr = label; // Store the created object in the global pointer
}

void setup() {
    Serial.begin(115200);

    // Initialize the display.
    screen.init();

    // Set a light gray background so the white ball is easier to see.
    lv_obj_set_style_bg_color(lv_scr_act(), lv_palette_lighten(LV_PALETTE_GREY, 4), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    // -------------------------------------------------
    // 1. Create the ball
    // -------------------------------------------------
    obj_ball = lv_obj_create(lv_scr_act());
    lv_obj_set_size(obj_ball, 80, 80);          // Set size to 80x80
    lv_obj_set_style_radius(obj_ball, LV_RADIUS_CIRCLE, 0); // Make it circular
    lv_obj_align(obj_ball, LV_ALIGN_TOP_MID, 0, 30);        // Top-center of the screen

    // Initial color: white (R255 G255 B255)
    lv_obj_set_style_bg_color(obj_ball, lv_color_white(), 0);

    // Add the title label.
    lv_obj_t * label_title = lv_label_create(lv_scr_act());
    lv_label_set_text(label_title, "RGB Color Mixer");
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 5);

    // -------------------------------------------------
    // 2. Create the three sliders
    // -------------------------------------------------
    // Red slider (Y offset 20) - first position
    create_color_slider(lv_palette_main(LV_PALETTE_RED), 20, &slider_r, &label_val_r);

    // Green slider (Y offset 60) - second position
    create_color_slider(lv_palette_main(LV_PALETTE_GREEN), 60, &slider_g, &label_val_g);

    // Blue slider (Y offset 100) - third position
    create_color_slider(lv_palette_main(LV_PALETTE_BLUE), 100, &slider_b, &label_val_b);

    // Add R/G/B labels to the left of the sliders.
    lv_obj_t* l_r = lv_label_create(lv_scr_act()); lv_label_set_text(l_r, "R"); lv_obj_align_to(l_r, slider_r, LV_ALIGN_OUT_LEFT_MID, -10, 0);
    lv_obj_t* l_g = lv_label_create(lv_scr_act()); lv_label_set_text(l_g, "G"); lv_obj_align_to(l_g, slider_g, LV_ALIGN_OUT_LEFT_MID, -10, 0);
    lv_obj_t* l_b = lv_label_create(lv_scr_act()); lv_label_set_text(l_b, "B"); lv_obj_align_to(l_b, slider_b, LV_ALIGN_OUT_LEFT_MID, -10, 0);
}

void loop() {
    screen.routine();
    delay(5);
}
