#include "display.h"
#include "image_data.h"

// Buzzer pin definition
#define BUZZER_PIN 45

// Buzzer type selection
// true  = active buzzer, controlled directly by HIGH/LOW
// false = passive buzzer, requires a PWM square wave
#define ACTIVE_BUZZER false  // Change to false if you are using a passive buzzer

// Passive buzzer parameters (used only when ACTIVE_BUZZER = false)
#define BUZZER_FREQ 2000    // Frequency: 2 kHz
#define BUZZER_CHANNEL 0    // PWM channel
#define BUZZER_RESOLUTION 8 // PWM resolution: 8-bit

Display screen;

// Global variables
static bool buzzer_on = false;
static lv_obj_t * img_btn_buzzer;  // Buzzer button (left)
static lv_obj_t * img_btn_mute;    // Mute button (right)

/**
 * @brief Control the buzzer
 */
void setBuzzer(bool state) {
    buzzer_on = state;

    if(ACTIVE_BUZZER) {
        // Active buzzer: direct HIGH/LOW control
        digitalWrite(BUZZER_PIN, state ? HIGH : LOW);
    } else {
        // Passive buzzer: use PWM
        if(state) {
            ledcWriteTone(BUZZER_PIN, BUZZER_FREQ);  // Output tone frequency
        } else {
            ledcWriteTone(BUZZER_PIN, 0);  // Stop output
        }
    }

}

/**
 * @brief Buzzer button event callback (left)
 */
static void buzzer_btn_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        // Turn on the buzzer
        setBuzzer(true);
        Serial.println("Buzzer button clicked: ON");
    }
}

/**
 * @brief Mute button event callback (right)
 */
static void mute_btn_event_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        // Turn off the buzzer
        setBuzzer(false);
        Serial.println("Mute button clicked: OFF");
    }
}

void setup() {
    Serial.begin(115200);

    // Initialize the buzzer
    if(ACTIVE_BUZZER) {
        // Active buzzer: standard GPIO mode
        pinMode(BUZZER_PIN, OUTPUT);
        digitalWrite(BUZZER_PIN, LOW);
        Serial.println("Buzzer Type: Active");
    } else {
        // Passive buzzer: PWM mode
        ledcAttach(BUZZER_PIN, BUZZER_FREQ, BUZZER_RESOLUTION);
        ledcWriteTone(BUZZER_PIN, 0);  // Initially off
        Serial.println("Buzzer Type: Passive");
        Serial.printf("Frequency: %d Hz\n", BUZZER_FREQ);
    }

    Serial.println("\n========================================");
    Serial.println("06_LVGL_IMG_BTN - Image Button Demo");
    Serial.println("========================================\n");

    // Initialize the display driver
    screen.init();

    // Set the background to white
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_white(), 0);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

    // Create the title
    lv_obj_t * label_title = lv_label_create(lv_scr_act());
    lv_label_set_text(label_title, "Buzzer Control");
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 10);

    // Title style
    static lv_style_t style_title;
    lv_style_init(&style_title);
    lv_style_set_text_font(&style_title, &lv_font_montserrat_20);
    lv_style_set_text_color(&style_title, lv_color_black());
    lv_obj_add_style(label_title, &style_title, 0);

    // Create the left buzzer button using lv_img instead of lv_imgbtn
    img_btn_buzzer = lv_img_create(lv_scr_act());
    lv_img_set_src(img_btn_buzzer, &buzzer);
    lv_obj_align(img_btn_buzzer, LV_ALIGN_CENTER, -60, 0);  // Left offset: -60
    lv_obj_add_flag(img_btn_buzzer, LV_OBJ_FLAG_CLICKABLE);  // Make the image clickable
    lv_obj_add_event_cb(img_btn_buzzer, buzzer_btn_event_handler, LV_EVENT_CLICKED, NULL);

    // Create the right mute button using lv_img instead of lv_imgbtn
    img_btn_mute = lv_img_create(lv_scr_act());
    lv_img_set_src(img_btn_mute, &mute);
    lv_obj_align(img_btn_mute, LV_ALIGN_CENTER, 60, 0);  // Right offset: +60
    lv_obj_add_flag(img_btn_mute, LV_OBJ_FLAG_CLICKABLE);  // Make the image clickable
    lv_obj_add_event_cb(img_btn_mute, mute_btn_event_handler, LV_EVENT_CLICKED, NULL);


    // Add the label for the left button
    lv_obj_t * label_buzzer = lv_label_create(lv_scr_act());
    lv_label_set_text(label_buzzer, "ON");
    lv_obj_align(label_buzzer, LV_ALIGN_CENTER, -60, 80);
    lv_obj_set_style_text_color(label_buzzer, lv_color_make(0, 150, 0), 0);

    // Add the label for the right button
    lv_obj_t * label_mute_btn = lv_label_create(lv_scr_act());
    lv_label_set_text(label_mute_btn, "OFF");
    lv_obj_align(label_mute_btn, LV_ALIGN_CENTER, 60, 80);
    lv_obj_set_style_text_color(label_mute_btn, lv_color_make(150, 0, 0), 0);

    Serial.println("Setup complete!");
    Serial.println("Left button: Turn ON buzzer");
    Serial.println("Right button: Turn OFF buzzer\n");
}

void loop() {
    screen.routine();
    delay(5);
}
