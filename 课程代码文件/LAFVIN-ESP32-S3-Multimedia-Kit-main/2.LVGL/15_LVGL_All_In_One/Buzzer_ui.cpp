#include "Buzzer_ui.h"

#include "all_in_one_app.h"

#include <stdint.h>

namespace {

struct PianoKey {
  const char *name;
  uint16_t frequency;
};

constexpr uint8_t kPwmResolution = 8;

constexpr int kExitButtonX = 12;
constexpr int kExitButtonY = 10;
constexpr int kExitButtonW = 44;
constexpr int kExitButtonH = 32;

constexpr int kPanelX = 16;
constexpr int kPanelY = 44;
constexpr int kPanelW = 238;
constexpr int kPanelH = 150;

constexpr int kKeyWidth = 34;
constexpr int kKeyHeight = 150;
constexpr int kBlackKeyWidth = 22;
constexpr int kBlackKeyHeight = 82;

constexpr int kBlackKeyX[BLACK_KEY_COUNT] = {23, 57, 125, 159, 193};

constexpr int kVolumeSliderX = 286;
constexpr int kVolumeSliderY = 58;
constexpr int kVolumeSliderW = 10;
constexpr int kVolumeSliderH = 116;

constexpr int kSwitchLabelX = 120;
constexpr int kSwitchLabelY = 206;
constexpr int kSwitchX = 180;
constexpr int kSwitchY = 202;

constexpr PianoKey kPianoKeys[PIANO_KEY_COUNT] = {
  {"c1", 262},
  {"d1", 294},
  {"e1", 330},
  {"f1", 349},
  {"g1", 392},
  {"a1", 440},
  {"b1", 494},
};

bool s_buzzer_enabled = false;
uint8_t s_volume = 120;
int8_t s_active_key = -1;

static lv_style_t s_screen_style;
static lv_style_t s_exit_style;
static lv_style_t s_exit_pressed_style;
static lv_style_t s_panel_style;
static lv_style_t s_key_style;
static lv_style_t s_key_pressed_style;
static lv_style_t s_black_key_style;
static lv_style_t s_switch_label_style;
static bool s_styles_ready = false;

static void create_styles(void);
static void setup_header(BuzzerUI *ui);
static void setup_piano(BuzzerUI *ui);
static void setup_volume(BuzzerUI *ui);
static void setup_power(BuzzerUI *ui);

static void buzzer_init(void);
static void buzzer_play_frequency(uint16_t frequency);
static void buzzer_stop_output(void);
static void update_power_ui(void);

static void exit_event_handler(lv_event_t *e);
static void key_event_handler(lv_event_t *e);
static void volume_slider_event_handler(lv_event_t *e);
static void power_switch_event_handler(lv_event_t *e);

void create_styles(void) {
  if (s_styles_ready) {
    return;
  }

  lv_style_init(&s_screen_style);
  lv_style_set_bg_color(&s_screen_style, lv_color_hex(0xF8FAFC));
  lv_style_set_bg_opa(&s_screen_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_screen_style, 0);
  lv_style_set_pad_all(&s_screen_style, 0);

  lv_style_init(&s_exit_style);
  lv_style_set_bg_opa(&s_exit_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_exit_style, 0);
  lv_style_set_shadow_width(&s_exit_style, 0);
  lv_style_set_outline_width(&s_exit_style, 0);
  lv_style_set_text_color(&s_exit_style, lv_color_hex(0x2793E6));
  lv_style_set_text_font(&s_exit_style, &lv_font_montserrat_24);
  lv_style_set_pad_all(&s_exit_style, 0);

  lv_style_init(&s_exit_pressed_style);
  lv_style_set_translate_y(&s_exit_pressed_style, 2);
  lv_style_set_text_opa(&s_exit_pressed_style, LV_OPA_70);

  lv_style_init(&s_panel_style);
  lv_style_set_bg_color(&s_panel_style, lv_color_white());
  lv_style_set_bg_opa(&s_panel_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_panel_style, 2);
  lv_style_set_border_color(&s_panel_style, lv_color_hex(0x111111));
  lv_style_set_radius(&s_panel_style, 0);
  lv_style_set_pad_all(&s_panel_style, 0);

  lv_style_init(&s_key_style);
  lv_style_set_bg_color(&s_key_style, lv_color_white());
  lv_style_set_bg_opa(&s_key_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_key_style, 2);
  lv_style_set_border_color(&s_key_style, lv_color_hex(0x111111));
  lv_style_set_radius(&s_key_style, 0);
  lv_style_set_shadow_width(&s_key_style, 0);
  lv_style_set_text_color(&s_key_style, lv_color_hex(0x111111));
  lv_style_set_text_font(&s_key_style, &lv_font_montserrat_18);
  lv_style_set_pad_all(&s_key_style, 0);
  lv_style_set_pad_bottom(&s_key_style, 12);

  lv_style_init(&s_key_pressed_style);
  lv_style_set_bg_color(&s_key_pressed_style, lv_color_hex(0xDCEEFB));
  lv_style_set_text_color(&s_key_pressed_style, lv_color_hex(0x1F5F8B));

  lv_style_init(&s_black_key_style);
  lv_style_set_bg_color(&s_black_key_style, lv_color_hex(0x050505));
  lv_style_set_bg_opa(&s_black_key_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_black_key_style, 0);
  lv_style_set_radius(&s_black_key_style, 0);
  lv_style_set_shadow_width(&s_black_key_style, 10);
  lv_style_set_shadow_color(&s_black_key_style, lv_color_hex(0x6B7280));
  lv_style_set_shadow_ofs_y(&s_black_key_style, 2);

  lv_style_init(&s_switch_label_style);
  lv_style_set_text_color(&s_switch_label_style, lv_color_hex(0x334E68));
  lv_style_set_text_font(&s_switch_label_style, &lv_font_montserrat_14);

  s_styles_ready = true;
}

void setup_header(BuzzerUI *ui) {
  ui->exit_button = lv_btn_create(ui->screen);
  lv_obj_set_pos(ui->exit_button, kExitButtonX, kExitButtonY);
  lv_obj_set_size(ui->exit_button, kExitButtonW, kExitButtonH);
  lv_obj_add_style(ui->exit_button, &s_exit_style, LV_PART_MAIN);
  lv_obj_add_style(ui->exit_button, &s_exit_pressed_style, LV_STATE_PRESSED);
  lv_obj_add_event_cb(ui->exit_button, exit_event_handler, LV_EVENT_CLICKED, nullptr);

  ui->exit_label = lv_label_create(ui->exit_button);
  lv_label_set_text(ui->exit_label, LV_SYMBOL_LEFT);
  lv_obj_center(ui->exit_label);
}

void setup_piano(BuzzerUI *ui) {
  ui->piano_panel = lv_obj_create(ui->screen);
  lv_obj_set_pos(ui->piano_panel, kPanelX, kPanelY);
  lv_obj_set_size(ui->piano_panel, kPanelW, kPanelH);
  lv_obj_add_style(ui->piano_panel, &s_panel_style, LV_PART_MAIN);
  lv_obj_clear_flag(ui->piano_panel, LV_OBJ_FLAG_SCROLLABLE);

  for (uint8_t i = 0; i < PIANO_KEY_COUNT; ++i) {
    ui->key_buttons[i] = lv_btn_create(ui->piano_panel);
    lv_obj_set_pos(ui->key_buttons[i], i * kKeyWidth, 0);
    lv_obj_set_size(ui->key_buttons[i], kKeyWidth, kKeyHeight);
    lv_obj_add_style(ui->key_buttons[i], &s_key_style, LV_PART_MAIN);
    lv_obj_add_style(ui->key_buttons[i], &s_key_pressed_style, LV_STATE_PRESSED);
    lv_obj_add_event_cb(
      ui->key_buttons[i],
      key_event_handler,
      LV_EVENT_ALL,
      reinterpret_cast<void *>(static_cast<uintptr_t>(i)));

    lv_obj_t *label = lv_label_create(ui->key_buttons[i]);
    lv_label_set_text(label, kPianoKeys[i].name);
    lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -6);
  }

  // Decorative black keys keep the first lesson simple while making the piano
  // look much closer to the real instrument layout.
  for (uint8_t i = 0; i < BLACK_KEY_COUNT; ++i) {
    ui->black_keys[i] = lv_obj_create(ui->piano_panel);
    lv_obj_set_pos(ui->black_keys[i], kBlackKeyX[i], 0);
    lv_obj_set_size(ui->black_keys[i], kBlackKeyWidth, kBlackKeyHeight);
    lv_obj_add_style(ui->black_keys[i], &s_black_key_style, LV_PART_MAIN);
    lv_obj_clear_flag(ui->black_keys[i], LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ui->black_keys[i], LV_OBJ_FLAG_CLICKABLE);
  }
}

void setup_volume(BuzzerUI *ui) {
  ui->volume_slider = lv_slider_create(ui->screen);
  lv_obj_set_pos(ui->volume_slider, kVolumeSliderX, kVolumeSliderY);
  lv_obj_set_size(ui->volume_slider, kVolumeSliderW, kVolumeSliderH);
  lv_slider_set_range(ui->volume_slider, 0, 255);
  lv_slider_set_value(ui->volume_slider, s_volume, LV_ANIM_OFF);
  lv_slider_set_mode(ui->volume_slider, LV_SLIDER_MODE_NORMAL);
  lv_obj_set_style_bg_color(ui->volume_slider, lv_palette_lighten(LV_PALETTE_GREY, 3), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui->volume_slider, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(ui->volume_slider, LV_RADIUS_CIRCLE, LV_PART_MAIN);

  lv_obj_set_style_bg_color(ui->volume_slider, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(ui->volume_slider, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_radius(ui->volume_slider, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);

  lv_obj_set_style_bg_color(ui->volume_slider, lv_color_white(), LV_PART_KNOB);
  lv_obj_set_style_border_color(ui->volume_slider, lv_palette_main(LV_PALETTE_BLUE), LV_PART_KNOB);
  lv_obj_set_style_border_width(ui->volume_slider, 2, LV_PART_KNOB);
  lv_obj_set_style_pad_all(ui->volume_slider, 6, LV_PART_KNOB);  
  lv_obj_set_style_radius(ui->volume_slider, LV_RADIUS_CIRCLE, LV_PART_KNOB);
  
  lv_obj_add_flag(ui->volume_slider, LV_OBJ_FLAG_ADV_HITTEST);
  lv_obj_add_event_cb(ui->volume_slider, volume_slider_event_handler, LV_EVENT_VALUE_CHANGED, nullptr);
}

void setup_power(BuzzerUI *ui) {
  ui->power_label = lv_label_create(ui->screen);
  lv_obj_set_pos(ui->power_label, kSwitchLabelX, kSwitchLabelY);
  lv_label_set_text(ui->power_label, "Buzzer");
  lv_obj_add_style(ui->power_label, &s_switch_label_style, LV_PART_MAIN);

  ui->power_switch = lv_switch_create(ui->screen);
  lv_obj_set_pos(ui->power_switch, kSwitchX, kSwitchY);
  lv_obj_add_event_cb(ui->power_switch, power_switch_event_handler, LV_EVENT_VALUE_CHANGED, nullptr);
}

void buzzer_init(void) {
  ledcAttach(BUZZER_PIN, 1000, kPwmResolution);
  ledcWriteTone(BUZZER_PIN, 0);
  ledcWrite(BUZZER_PIN, 0);
}

void buzzer_play_frequency(uint16_t frequency) {
  if (!s_buzzer_enabled || s_volume == 0 || frequency == 0) {
    buzzer_stop_output();
    return;
  }

  ledcWriteTone(BUZZER_PIN, frequency);
  ledcWrite(BUZZER_PIN, s_volume);
}

void buzzer_stop_output(void) {
  ledcWriteTone(BUZZER_PIN, 0);
  ledcWrite(BUZZER_PIN, 0);
}

void update_power_ui(void) {
  if (s_buzzer_enabled) {
    lv_obj_add_state(g_buzzer_ui.power_switch, LV_STATE_CHECKED);
  } else {
    lv_obj_clear_state(g_buzzer_ui.power_switch, LV_STATE_CHECKED);
  }
}

void exit_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  s_active_key = -1;
  s_buzzer_enabled = false;
  update_power_ui();
  buzzer_stop_output();
  all_in_one_show_home();
}

void key_event_handler(lv_event_t *e) {
  const uint8_t key_index = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
  const lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_PRESSED) {
    s_active_key = static_cast<int8_t>(key_index);
    buzzer_play_frequency(kPianoKeys[key_index].frequency);
    return;
  }

  if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    if (s_active_key == static_cast<int8_t>(key_index)) {
      s_active_key = -1;
      buzzer_stop_output();
    }
  }
}

void volume_slider_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
    return;
  }

  s_volume = static_cast<uint8_t>(lv_slider_get_value(g_buzzer_ui.volume_slider));

  if (s_active_key >= 0) {
    buzzer_play_frequency(kPianoKeys[s_active_key].frequency);
  } else {
    buzzer_stop_output();
  }
}

void power_switch_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
    return;
  }

  s_buzzer_enabled = lv_obj_has_state(g_buzzer_ui.power_switch, LV_STATE_CHECKED);

  if (!s_buzzer_enabled || s_active_key < 0) {
    buzzer_stop_output();
    return;
  }

  buzzer_play_frequency(kPianoKeys[s_active_key].frequency);
}

}  // namespace

BuzzerUI g_buzzer_ui = {};

void buzzer_ui_setup(BuzzerUI *ui) {
  create_styles();
  buzzer_init();
  buzzer_stop_output();

  ui->screen = lv_obj_create(nullptr);
  lv_obj_add_style(ui->screen, &s_screen_style, LV_PART_MAIN);
  lv_obj_clear_flag(ui->screen, LV_OBJ_FLAG_SCROLLABLE);

  setup_header(ui);
  setup_piano(ui);
  setup_volume(ui);
  setup_power(ui);
  update_power_ui();

  Serial.println("Buzzer UI ready.");
}

void buzzer_ui_loop(void) {
}
