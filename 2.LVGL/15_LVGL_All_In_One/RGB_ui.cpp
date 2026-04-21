#include "RGB_ui.h"

#include "all_in_one_app.h"

#include <stdint.h>

namespace {

// Available LED animation modes for this demo page.
enum class RgbMode : uint8_t {
  Static,
  Breath,
  Blink,
  Rainbow,
};

// A preset contains both the LED RGB value and the button color shown on screen.
struct RgbPreset {
  const char *name;
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint32_t button_color_hex;
};

// Preset colors shown as quick-access buttons.
constexpr RgbPreset kPresets[] = {
  {"Red", 255, 0, 0, 0xF04E4E},
  {"Green", 0, 255, 0, 0x3CCB7F},
  {"Blue", 0, 110, 255, 0x3A86FF},
  {"Yellow", 255, 210, 0, 0xF4C542},
  {"Purple", 186, 85, 211, 0xA855F7},
  {"White", 255, 255, 255, 0xD9E1F2},
};

constexpr const char *kModeNames[] = {
  "Static",
  "Breath",
  "Blink",
  "Rainbow",
};

// UI layout constants.
constexpr int kExitButtonX = 12;
constexpr int kExitButtonY = 10;
constexpr int kExitButtonW = 44;
constexpr int kExitButtonH = 32;

constexpr int kTitleX = 68;
constexpr int kTitleY = 10;
constexpr int kTitleW = 220;
constexpr int kTitleH = 28;

constexpr int kSubtitleX = 68;
constexpr int kSubtitleY = 36;
constexpr int kSubtitleW = 236;
constexpr int kSubtitleH = 18;

constexpr int kPreviewX = 16;
constexpr int kPreviewY = 62;
constexpr int kPreviewW = 288;
constexpr int kPreviewH = 84;

constexpr int kPresetY = 154;
constexpr int kPresetW = 44;
constexpr int kPresetH = 36;
constexpr int kPresetGap = 4;
constexpr int kPresetStartX = 16;

constexpr int kModeY = 198;
constexpr int kModeW = 68;
constexpr int kModeH = 30;
constexpr int kModeGap = 6;
constexpr int kModeStartX = 16;

constexpr int kSliderX = 80;
constexpr int kSliderY = 20;
constexpr int kSliderW = 170;
constexpr int kSliderH = 8;

constexpr int kBrightnessValueX = 260;
constexpr int kBrightnessValueY = 18;
constexpr int kBrightnessValueW = 86;
constexpr int kBrightnessValueH = 22;

// UI refresh cadence for animation and preview updates.
constexpr uint32_t kUiRefreshMs = 16;

// Current UI/LED runtime state.
volatile bool s_ui_dirty = true;
RgbMode s_current_mode = RgbMode::Static;
uint8_t s_brightness = 64;
uint8_t s_base_r = 255;
uint8_t s_base_g = 0;
uint8_t s_base_b = 0;
uint32_t s_last_anim_ms = 0;
uint32_t s_last_ui_ms = 0;
bool s_blink_on = true;
uint8_t s_rainbow_hue = 0;
int s_breath_direction = 1;
uint8_t s_breath_level = 64;

static void rgb_exit_event_handler(lv_event_t *e);
static void preset_button_event_handler(lv_event_t *e);
static void mode_button_event_handler(lv_event_t *e);
static void brightness_slider_event_handler(lv_event_t *e);

static void create_styles(void);
static void setup_header(RGBUI *ui);
static void setup_preview(RGBUI *ui);
static void setup_presets(RGBUI *ui);
static void setup_modes(RGBUI *ui);
static void setup_slider(RGBUI *ui);
static void refresh_ui(void);
static void refresh_preview(void);
static void refresh_mode_buttons(void);
static void write_led(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
static lv_color_t current_preview_color(void);
static const char *current_color_name(void);
static void apply_current_led(void);
static void update_animation(void);
static void select_preset(uint8_t index);
static void select_mode(RgbMode mode);
static lv_color_t hsv_to_lv_color(uint8_t hue, uint8_t sat, uint8_t val);
static void hsv_to_rgb(uint8_t hue, uint8_t sat, uint8_t val, uint8_t *r, uint8_t *g, uint8_t *b);

static lv_style_t s_screen_style;
static lv_style_t s_exit_style;
static lv_style_t s_exit_pressed_style;
static lv_style_t s_title_style;
static lv_style_t s_subtitle_style;
static lv_style_t s_preview_style;
static lv_style_t s_preview_name_style;
static lv_style_t s_mode_style;
static lv_style_t s_mode_checked_style;
static lv_style_t s_brightness_style;
static bool s_styles_ready = false;

// Build the reusable LVGL styles used across this screen.
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

  lv_style_init(&s_title_style);
  lv_style_set_text_color(&s_title_style, lv_color_hex(0x102A43));
  lv_style_set_text_font(&s_title_style, &lv_font_montserrat_22);

  lv_style_init(&s_subtitle_style);
  lv_style_set_text_color(&s_subtitle_style, lv_color_hex(0x52606D));
  lv_style_set_text_font(&s_subtitle_style, &lv_font_montserrat_12);

  lv_style_init(&s_preview_style);
  lv_style_set_bg_color(&s_preview_style, lv_color_white());
  lv_style_set_bg_opa(&s_preview_style, LV_OPA_COVER);
  lv_style_set_radius(&s_preview_style, 18);
  lv_style_set_border_width(&s_preview_style, 0);
  lv_style_set_shadow_width(&s_preview_style, 16);
  lv_style_set_shadow_color(&s_preview_style, lv_color_hex(0xD9E2EC));
  lv_style_set_shadow_opa(&s_preview_style, LV_OPA_70);

  lv_style_init(&s_preview_name_style);
  lv_style_set_text_color(&s_preview_name_style, lv_color_hex(0x102A43));
  lv_style_set_text_font(&s_preview_name_style, &lv_font_montserrat_16);

  lv_style_init(&s_mode_style);
  lv_style_set_bg_color(&s_mode_style, lv_color_white());
  lv_style_set_bg_opa(&s_mode_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_mode_style, 1);
  lv_style_set_border_color(&s_mode_style, lv_color_hex(0xD9E2EC));
  lv_style_set_radius(&s_mode_style, 14);
  lv_style_set_text_color(&s_mode_style, lv_color_hex(0x334E68));
  lv_style_set_shadow_width(&s_mode_style, 0);

  lv_style_init(&s_mode_checked_style);
  lv_style_set_bg_color(&s_mode_checked_style, lv_color_hex(0xDCEEFB));
  lv_style_set_border_color(&s_mode_checked_style, lv_color_hex(0x3E7BFA));
  lv_style_set_text_color(&s_mode_checked_style, lv_color_hex(0x1F3A8A));

  lv_style_init(&s_brightness_style);
  lv_style_set_text_color(&s_brightness_style, lv_color_hex(0x486581));
  lv_style_set_text_font(&s_brightness_style, &lv_font_montserrat_14);

  s_styles_ready = true;
}

// Top row: exit button and the brightness control.
void setup_header(RGBUI *ui) {
  ui->exit_button = lv_btn_create(ui->screen);
  lv_obj_set_pos(ui->exit_button, kExitButtonX, kExitButtonY);
  lv_obj_set_size(ui->exit_button, kExitButtonW, kExitButtonH);
  lv_obj_add_style(ui->exit_button, &s_exit_style, LV_PART_MAIN);
  lv_obj_add_style(ui->exit_button, &s_exit_pressed_style, LV_STATE_PRESSED);
  lv_obj_add_event_cb(ui->exit_button, rgb_exit_event_handler, LV_EVENT_CLICKED, nullptr);

  ui->exit_label = lv_label_create(ui->exit_button);
  lv_label_set_text(ui->exit_label, LV_SYMBOL_LEFT);
  lv_obj_center(ui->exit_label);

}

// Center preview card that mirrors the current LED state.
void setup_preview(RGBUI *ui) {
  ui->preview_panel = lv_obj_create(ui->screen);
  lv_obj_set_pos(ui->preview_panel, kPreviewX, kPreviewY);
  lv_obj_set_size(ui->preview_panel, kPreviewW, kPreviewH);
  lv_obj_add_style(ui->preview_panel, &s_preview_style, LV_PART_MAIN);
  lv_obj_clear_flag(ui->preview_panel, LV_OBJ_FLAG_SCROLLABLE);

  ui->preview_light = lv_obj_create(ui->preview_panel);
  lv_obj_set_size(ui->preview_light, 64, 64);
  lv_obj_align(ui->preview_light, LV_ALIGN_LEFT_MID, 18, 0);
  lv_obj_set_style_radius(ui->preview_light, LV_RADIUS_CIRCLE, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui->preview_light, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui->preview_light, 24, LV_PART_MAIN);
  lv_obj_set_style_shadow_opa(ui->preview_light, LV_OPA_80, LV_PART_MAIN);

  ui->preview_name = lv_label_create(ui->preview_panel);
  lv_obj_align(ui->preview_name, LV_ALIGN_LEFT_MID, 102, 0);
  lv_label_set_text(ui->preview_name, "Red / Static");
  lv_obj_add_style(ui->preview_name, &s_preview_name_style, LV_PART_MAIN);
}

// Quick preset color buttons.
void setup_presets(RGBUI *ui) {
  for (uint8_t i = 0; i < 6; ++i) {
    ui->preset_buttons[i] = lv_btn_create(ui->screen);
    lv_obj_set_pos(ui->preset_buttons[i], kPresetStartX + i * (kPresetW + kPresetGap), kPresetY);
    lv_obj_set_size(ui->preset_buttons[i], kPresetW, kPresetH);
    lv_obj_set_style_radius(ui->preset_buttons[i], 12, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui->preset_buttons[i], lv_color_hex(kPresets[i].button_color_hex), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui->preset_buttons[i], LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(ui->preset_buttons[i], 0, LV_PART_MAIN);
    lv_obj_add_event_cb(
      ui->preset_buttons[i],
      preset_button_event_handler,
      LV_EVENT_CLICKED,
      reinterpret_cast<void *>(static_cast<uintptr_t>(i)));

    lv_obj_t *label = lv_label_create(ui->preset_buttons[i]);
    lv_label_set_text(label, kPresets[i].name);
    lv_obj_set_style_text_color(
      label,
      (i == 5) ? lv_color_hex(0x334E68) : lv_color_white(),
      LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_obj_center(label);
  }
}

// Animation mode selector buttons.
void setup_modes(RGBUI *ui) {
  for (uint8_t i = 0; i < 4; ++i) {
    ui->mode_buttons[i] = lv_btn_create(ui->screen);
    lv_obj_set_pos(ui->mode_buttons[i], kModeStartX + i * (kModeW + kModeGap), kModeY);
    lv_obj_set_size(ui->mode_buttons[i], kModeW, kModeH);
    lv_obj_add_style(ui->mode_buttons[i], &s_mode_style, LV_PART_MAIN);
    lv_obj_add_style(ui->mode_buttons[i], &s_mode_checked_style, LV_STATE_CHECKED);
    lv_obj_add_flag(ui->mode_buttons[i], LV_OBJ_FLAG_CHECKABLE);
    lv_obj_add_event_cb(
      ui->mode_buttons[i],
      mode_button_event_handler,
      LV_EVENT_CLICKED,
      reinterpret_cast<void *>(static_cast<uintptr_t>(i)));

    lv_obj_t *label = lv_label_create(ui->mode_buttons[i]);
    lv_label_set_text(label, kModeNames[i]);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_center(label);
  }
}

// One brightness slider keeps this page simpler than the 4-slider Sketch_21 page.
void setup_slider(RGBUI *ui) {
  ui->brightness_slider = lv_slider_create(ui->screen);
  lv_obj_set_pos(ui->brightness_slider, kSliderX, kSliderY);
  lv_obj_set_size(ui->brightness_slider, kSliderW, kSliderH);
  lv_slider_set_range(ui->brightness_slider, 0, 255);
  lv_slider_set_value(ui->brightness_slider, s_brightness, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(ui->brightness_slider, lv_color_hex(0xDCEEFB), LV_PART_MAIN);
  lv_obj_set_style_bg_color(ui->brightness_slider, lv_color_hex(0x3E7BFA), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(ui->brightness_slider, lv_color_white(), LV_PART_KNOB);
  lv_obj_set_style_border_color(ui->brightness_slider, lv_color_hex(0x3E7BFA), LV_PART_KNOB);
  lv_obj_set_style_border_width(ui->brightness_slider, 3, LV_PART_KNOB);
  lv_obj_add_event_cb(ui->brightness_slider, brightness_slider_event_handler, LV_EVENT_VALUE_CHANGED, nullptr);

  ui->brightness_value = lv_label_create(ui->screen);
  lv_obj_set_pos(ui->brightness_value, kBrightnessValueX, kBrightnessValueY);
  lv_obj_set_size(ui->brightness_value, kBrightnessValueW, kBrightnessValueH);
  lv_obj_add_style(ui->brightness_value, &s_brightness_style, LV_PART_MAIN);
}

// Write the current RGB value to the built-in WS2812 with brightness scaling.
void write_led(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
  const uint8_t r_scaled = static_cast<uint8_t>((static_cast<uint16_t>(r) * brightness) / 255);
  const uint8_t g_scaled = static_cast<uint8_t>((static_cast<uint16_t>(g) * brightness) / 255);
  const uint8_t b_scaled = static_cast<uint8_t>((static_cast<uint16_t>(b) * brightness) / 255);
  neopixelWrite(RGB_LED_PIN, r_scaled, g_scaled, b_scaled);
}

// Convert HSV to an LVGL color for the preview widget.
lv_color_t hsv_to_lv_color(uint8_t hue, uint8_t sat, uint8_t val) {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
  hsv_to_rgb(hue, sat, val, &r, &g, &b);
  return lv_color_make(r, g, b);
}

// Convert HSV to raw RGB bytes for the rainbow effect.
void hsv_to_rgb(uint8_t hue, uint8_t sat, uint8_t val, uint8_t *r, uint8_t *g, uint8_t *b) {
  const uint8_t region = hue / 43;
  const uint8_t remainder = (hue - (region * 43)) * 6;

  const uint8_t p = (val * (255 - sat)) >> 8;
  const uint8_t q = (val * (255 - ((sat * remainder) >> 8))) >> 8;
  const uint8_t t = (val * (255 - ((sat * (255 - remainder)) >> 8))) >> 8;

  switch (region) {
    case 0:
      *r = val; *g = t; *b = p;
      return;
    case 1:
      *r = q; *g = val; *b = p;
      return;
    case 2:
      *r = p; *g = val; *b = t;
      return;
    case 3:
      *r = p; *g = q; *b = val;
      return;
    case 4:
      *r = t; *g = p; *b = val;
      return;
    default:
      *r = val; *g = p; *b = q;
      return;
  }
}

// Return the color currently shown by the on-screen preview.
lv_color_t current_preview_color(void) {
  uint8_t r = s_base_r;
  uint8_t g = s_base_g;
  uint8_t b = s_base_b;

  switch (s_current_mode) {
    case RgbMode::Static:
      return lv_color_make(r, g, b);
    case RgbMode::Breath: {
      r = static_cast<uint8_t>((static_cast<uint16_t>(r) * s_breath_level) / 255);
      g = static_cast<uint8_t>((static_cast<uint16_t>(g) * s_breath_level) / 255);
      b = static_cast<uint8_t>((static_cast<uint16_t>(b) * s_breath_level) / 255);
      return lv_color_make(r, g, b);
    }
    case RgbMode::Blink:
      return s_blink_on ? lv_color_make(r, g, b) : lv_color_make(18, 24, 32);
    case RgbMode::Rainbow:
      return hsv_to_lv_color(s_rainbow_hue, 255, 255);
    default:
      return lv_color_make(r, g, b);
  }
}

// Apply the active mode to the real LED.
void apply_current_led(void) {
  switch (s_current_mode) {
    case RgbMode::Static:
      write_led(s_base_r, s_base_g, s_base_b, s_brightness);
      break;
    case RgbMode::Breath:
      write_led(s_base_r, s_base_g, s_base_b, s_breath_level);
      break;
    case RgbMode::Blink:
      if (s_blink_on) {
        write_led(s_base_r, s_base_g, s_base_b, s_brightness);
      } else {
        write_led(0, 0, 0, 0);
      }
      break;
    case RgbMode::Rainbow: {
      uint8_t r = 0;
      uint8_t g = 0;
      uint8_t b = 0;
      hsv_to_rgb(s_rainbow_hue, 255, 255, &r, &g, &b);
      write_led(r, g, b, s_brightness);
      break;
    }
  }
}

// Human-readable name for the current base color.
const char *current_color_name(void) {
  if (s_current_mode == RgbMode::Rainbow) {
    return "Rainbow";
  }

  for (const RgbPreset &preset : kPresets) {
    if (preset.r == s_base_r && preset.g == s_base_g && preset.b == s_base_b) {
      return preset.name;
    }
  }

  return "Custom";
}

// Update the preview panel color and caption.
void refresh_preview(void) {
  const lv_color_t preview = current_preview_color();
  lv_obj_set_style_bg_color(g_rgb_ui.preview_light, preview, LV_PART_MAIN);
  lv_obj_set_style_shadow_color(g_rgb_ui.preview_light, preview, LV_PART_MAIN);
  lv_label_set_text_fmt(
    g_rgb_ui.preview_name,
    "%s / %s",
    current_color_name(),
    kModeNames[static_cast<uint8_t>(s_current_mode)]);
}

// Highlight the currently selected mode button.
void refresh_mode_buttons(void) {
  for (uint8_t i = 0; i < 4; ++i) {
    if (i == static_cast<uint8_t>(s_current_mode)) {
      lv_obj_add_state(g_rgb_ui.mode_buttons[i], LV_STATE_CHECKED);
    } else {
      lv_obj_clear_state(g_rgb_ui.mode_buttons[i], LV_STATE_CHECKED);
    }
  }
}

// Push the latest state to labels, preview widgets, and the physical LED.
void refresh_ui(void) {
  if (!s_ui_dirty) {
    return;
  }

  lv_label_set_text_fmt(g_rgb_ui.brightness_value, "%d", s_brightness);
  refresh_mode_buttons();
  refresh_preview();
  apply_current_led();
  s_ui_dirty = false;
}

// Update animation state for dynamic modes.
void update_animation(void) {
  const uint32_t now = millis();

  switch (s_current_mode) {
    case RgbMode::Static:
      return;
    case RgbMode::Breath:
      if (now - s_last_anim_ms < 18) {
        return;
      }
      s_last_anim_ms = now;
      if (s_breath_direction > 0) {
        if (s_breath_level >= s_brightness) {
          s_breath_direction = -1;
        } else {
          s_breath_level = static_cast<uint8_t>(min(255, s_breath_level + 3));
        }
      } else {
        if (s_breath_level <= 8) {
          s_breath_direction = 1;
        } else {
          s_breath_level = static_cast<uint8_t>(s_breath_level - 3);
        }
      }
      s_ui_dirty = true;
      return;
    case RgbMode::Blink:
      if (now - s_last_anim_ms < 380) {
        return;
      }
      s_last_anim_ms = now;
      s_blink_on = !s_blink_on;
      s_ui_dirty = true;
      return;
    case RgbMode::Rainbow:
      if (now - s_last_anim_ms < 24) {
        return;
      }
      s_last_anim_ms = now;
      s_rainbow_hue += 2;
      s_ui_dirty = true;
      return;
  }
}

// Select one of the preset base colors.
void select_preset(uint8_t index) {
  if (index >= 6) {
    return;
  }

  s_base_r = kPresets[index].r;
  s_base_g = kPresets[index].g;
  s_base_b = kPresets[index].b;
  s_ui_dirty = true;
}

// Switch animation mode and re-seed mode-specific state.
void select_mode(RgbMode mode) {
  s_current_mode = mode;
  s_last_anim_ms = millis();
  s_blink_on = true;
  s_breath_level = (s_brightness / 2 < 16) ? 16 : static_cast<uint8_t>(s_brightness / 2);
  s_breath_direction = 1;
  s_ui_dirty = true;
}

// Placeholder exit action for later page navigation.
void rgb_exit_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  all_in_one_show_home();
}

// Preset color button callback.
void preset_button_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  const uintptr_t index = reinterpret_cast<uintptr_t>(lv_event_get_user_data(e));
  select_preset(static_cast<uint8_t>(index));
}

// Mode button callback.
void mode_button_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  const uintptr_t index = reinterpret_cast<uintptr_t>(lv_event_get_user_data(e));
  select_mode(static_cast<RgbMode>(index));
}

// Brightness slider callback.
void brightness_slider_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
    return;
  }

  s_brightness = static_cast<uint8_t>(lv_slider_get_value(g_rgb_ui.brightness_slider));
  s_ui_dirty = true;
}

}  // namespace

RGBUI g_rgb_ui = {};

// Build the complete RGB control page.
void rgb_ui_setup(RGBUI *ui) {
  create_styles();

  ui->screen = lv_obj_create(nullptr);
  lv_obj_add_style(ui->screen, &s_screen_style, LV_PART_MAIN);
  lv_obj_clear_flag(ui->screen, LV_OBJ_FLAG_SCROLLABLE);

  setup_header(ui);
  setup_preview(ui);
  setup_presets(ui);
  setup_modes(ui);
  setup_slider(ui);

  select_preset(0);
  select_mode(RgbMode::Static);
  refresh_ui();

  Serial.println("RGB UI ready.");
}

// Turn off the physical LED when leaving the RGB screen.
void rgb_ui_stop(void) {
  neopixelWrite(RGB_LED_PIN, 0, 0, 0);
}

// Main loop hook: animate first, then refresh the UI at a steady cadence.
void rgb_ui_loop(void) {
  update_animation();

  if (millis() - s_last_ui_ms >= kUiRefreshMs) {
    s_last_ui_ms = millis();
    refresh_ui();
  }
}
