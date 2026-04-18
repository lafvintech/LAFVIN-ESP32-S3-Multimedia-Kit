#include "display.h"

Display screen;

// Global variables
static int count_val = 0;
static lv_obj_t * label_count;   // Counter label
static lv_obj_t * label_comment; // Test text label

// Predefined colored text string.
// LVGL uses the #RRGGBB text# format for recoloring.
const char * colored_text =
    "#FF0000 t##FF8000 h##FFFF00 i##80FF00 s# "
    "#00FF00 i##00FF80 s# "
    "#00FFFF s##0080FF i##0000FF m##8000FF p##FF00FF l##FF0080 e# "
    "#FF0000 t##FF8000 e##FFFF00 s##80FF00 t# "
    "#00FF00 c##00FF80 o##00FFFF m##0080FF m##0000FF e##8000FF n##FF00FF t#";

// Shared event callback function
static void event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        // Get the user data ID passed by the button.
        intptr_t btn_id = (intptr_t)lv_event_get_user_data(e);

        switch (btn_id) {
            case 1: // Count +
                count_val++;
                lv_label_set_text_fmt(label_count, "Count: %d", count_val);
                break;

            case -1: // Count -
                count_val--;
                lv_label_set_text_fmt(label_count, "Count: %d", count_val);
                break;

            case 2: // Show Black Text
                // 1. Make sure recolor is disabled and return to normal text mode.
                lv_label_set_recolor(label_comment, false);
                // 2. Set plain text content.
                lv_label_set_text(label_comment, "this is simple test comment");
                // 3. Force black text in case a previous style remains.
                lv_obj_set_style_text_color(label_comment, lv_color_black(), 0);
                break;

            case 3: // Show Colorful Text
                // 1. Enable recoloring.
                lv_label_set_recolor(label_comment, true);
                // 2. Set the text string that contains embedded color codes.
                lv_label_set_text(label_comment, colored_text);
                break;
        }
    }
}

void setup() {
    Serial.begin(115200);

    // Initialize the display driver.
    screen.init();

    // 1. Set the background to white.
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_white(), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    // Define a shared large-font style.
    static lv_style_t style_big;
    lv_style_init(&style_big);
    lv_style_set_text_font(&style_big, &lv_font_montserrat_20); // Make sure montserrat_20 is enabled in lv_conf.h
    lv_style_set_text_color(&style_big, lv_color_black());

    // 2. Count label (top)
    label_count = lv_label_create(lv_scr_act());
    lv_label_set_text_fmt(label_count, "Count: %d", count_val);
    lv_obj_add_style(label_count, &style_big, 0);
    lv_obj_align(label_count, LV_ALIGN_TOP_MID, 0, 20);

    // 3. Comment label (below the count)
    // Start empty and reserve the area.
    label_comment = lv_label_create(lv_scr_act());
    lv_label_set_text(label_comment, "");
    lv_obj_set_width(label_comment, 220);  // Limit the width
    lv_label_set_long_mode(label_comment, LV_LABEL_LONG_WRAP); // Allow automatic wrapping

    // Set centered text alignment specifically for label_comment.
    static lv_style_t style_center;
    lv_style_init(&style_center);
    lv_style_set_text_align(&style_center, LV_TEXT_ALIGN_CENTER);
    lv_obj_add_style(label_comment, &style_big, 0);    // Apply font
    lv_obj_add_style(label_comment, &style_center, 0); // Apply alignment

    lv_obj_align(label_comment, LV_ALIGN_TOP_MID, 0, 55); // Place it below the count

    // 4. Increment/decrement buttons (upper-middle area)
    // Decrement button
    lv_obj_t * btn_minus = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn_minus, 70, 40);
    lv_obj_align(btn_minus, LV_ALIGN_CENTER, -50, -10);
    lv_obj_add_event_cb(btn_minus, event_handler, LV_EVENT_CLICKED, (void*)-1);

    lv_obj_t * l_min = lv_label_create(btn_minus);
    lv_label_set_text(l_min, "-");
    lv_obj_center(l_min);

    // Increment button
    lv_obj_t * btn_plus = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn_plus, 70, 40);
    lv_obj_align(btn_plus, LV_ALIGN_CENTER, 50, -10);
    lv_obj_add_event_cb(btn_plus, event_handler, LV_EVENT_CLICKED, (void*)1);

    lv_obj_t * l_plus = lv_label_create(btn_plus);
    lv_label_set_text(l_plus, "+");
    lv_obj_center(l_plus);

    // 5. Additional function buttons (lower half of the screen)
    // Button 3: Show Text
    lv_obj_t * btn_show = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn_show, 90, 40);
    lv_obj_align(btn_show, LV_ALIGN_CENTER, -55, 60); // Lower-left
    lv_obj_add_event_cb(btn_show, event_handler, LV_EVENT_CLICKED, (void*)2);

    lv_obj_t * l_show = lv_label_create(btn_show);
    lv_label_set_text(l_show, "Show");
    lv_obj_center(l_show);

    // Button 4: Colorize
    lv_obj_t * btn_color = lv_btn_create(lv_scr_act());
    lv_obj_set_size(btn_color, 90, 40);
    lv_obj_align(btn_color, LV_ALIGN_CENTER, 55, 60); // Lower-right
    lv_obj_add_event_cb(btn_color, event_handler, LV_EVENT_CLICKED, (void*)3);

    lv_obj_t * l_color = lv_label_create(btn_color);
    lv_label_set_text(l_color, "Color");
    lv_obj_center(l_color);
}

void loop() {
    screen.routine();
    delay(5);
}
