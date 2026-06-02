#include "hartrate_ui.h"

#include <Wire.h>
#include <string.h>

#include "MAX30105.h"
#include "heartRate.h"

namespace {

// ----------------------------
// Measurement behavior settings
// ----------------------------
// Keep this threshold aligned with Sketch_21 logic.
constexpr long kFingerDetectThreshold = 50000;
constexpr uint32_t kReportIntervalMs = 1000;

// ----------------------------
// Averaging window configuration
// ----------------------------
constexpr uint8_t kRateSize = 4;
constexpr uint8_t kWaveAverageSize = 10;
constexpr int32_t kWaveformGain = 3;

constexpr int32_t kChartMidpoint = (CHART_HIGH_LIMIT + CHART_LOW_LIMIT) / 2;

// ----------------------------
// UI layout constants
// ----------------------------
constexpr int kExitButtonX = 14;
constexpr int kExitButtonY = 10;
constexpr int kExitButtonW = 44;
constexpr int kExitButtonH = 32;

constexpr int kTitleLabelX = 100;
constexpr int kTitleLabelY = 12;
constexpr int kTitleLabelW = 120;
constexpr int kTitleLabelH = 28;

constexpr int kBpmLabelX = 240;
constexpr int kBpmLabelY = 12;
constexpr int kBpmLabelW = 54;
constexpr int kBpmLabelH = 28;

constexpr int kStatusLabelX = 140;
constexpr int kStatusLabelY = 38;
constexpr int kStatusLabelW = 160;
constexpr int kStatusLabelH = 22;

constexpr int kChartX = 58;
constexpr int kChartY = 56;
constexpr int kChartW = 238;
constexpr int kChartH = 164;

enum class HeartrateStatus {
  SensorError,
  PlaceFinger,
  DetectingPulse,
  Measuring,
};

// ----------------------------
// Sensor + chart runtime state
// ----------------------------
MAX30105 particleSensor;
lv_chart_series_t *s_chart_series = nullptr;

TaskHandle_t s_heartrate_task_handle = nullptr;
volatile bool s_heartrate_task_running = false;

volatile bool s_sensor_ready = false;
volatile bool s_finger_present = false;
volatile HeartrateStatus s_status = HeartrateStatus::PlaceFinger;

byte s_rates[kRateSize] = {0};
byte s_rate_spot = 0;
long s_last_beat_ms = 0;
volatile float s_beats_per_minute = 0.0f;
volatile int s_beat_average = 0;

long s_ir_values[kWaveAverageSize] = {0};
byte s_ir_spot = 0;
volatile long s_last_ir_value = 0;

uint32_t s_last_report_ms = 0;
volatile bool s_chart_reset_pending = false;
volatile bool s_chart_value_pending = false;
volatile lv_coord_t s_pending_chart_value = kChartMidpoint;

// ----------------------------
// Internal helpers
// ----------------------------
static void heartrate_exit_event_handler(lv_event_t *e);
static void create_styles(void);
static void setup_chart(HeartrateUI *ui);
static void setup_labels(HeartrateUI *ui);
static void setup_exit_button(HeartrateUI *ui);
static void init_sensor(void);
static void heartrate_shutdown(void);
static void heartrate_wake_up(void);
static void create_heartrate_task(void);
static void queue_chart_reset(void);
static void queue_chart_value(lv_coord_t value);
static void apply_pending_chart_updates(void);
static long average_ir_window(void);
static void seed_wave_history(long ir_value);
static void reset_chart(void);
static void reset_measurement_state(void);
static void update_status(HeartrateStatus status);
static const char *status_text(HeartrateStatus status);
static void refresh_ui(void);
static void print_status(void);
static void heartrate_task_loop(void *pvParameters);

static lv_style_t s_screen_style;
static lv_style_t s_chart_style;
static lv_style_t s_exit_button_style;
static lv_style_t s_exit_button_pressed_style;
static lv_style_t s_title_style;
static lv_style_t s_bpm_style;
static lv_style_t s_status_style;
static bool s_styles_ready = false;

// Build one-time LVGL styles shared by this screen.
void create_styles(void) {
  if (s_styles_ready) {
    return;
  }

  lv_style_init(&s_screen_style);
  lv_style_set_bg_color(&s_screen_style, lv_color_white());
  lv_style_set_bg_opa(&s_screen_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_screen_style, 0);
  lv_style_set_pad_all(&s_screen_style, 0);

  lv_style_init(&s_chart_style);
  lv_style_set_bg_color(&s_chart_style, lv_color_hex(0xF3F5F8));
  lv_style_set_bg_opa(&s_chart_style, LV_OPA_COVER);
  lv_style_set_border_width(&s_chart_style, 0);
  lv_style_set_radius(&s_chart_style, 0);
  lv_style_set_pad_all(&s_chart_style, 0);

  lv_style_init(&s_exit_button_style);
  lv_style_set_bg_opa(&s_exit_button_style, LV_OPA_TRANSP);
  lv_style_set_border_width(&s_exit_button_style, 0);
  lv_style_set_shadow_width(&s_exit_button_style, 0);
  lv_style_set_outline_width(&s_exit_button_style, 0);
  lv_style_set_text_color(&s_exit_button_style, lv_color_hex(0x2793E6));
  lv_style_set_text_font(&s_exit_button_style, &lv_font_montserrat_24);
  lv_style_set_pad_all(&s_exit_button_style, 0);

  lv_style_init(&s_exit_button_pressed_style);
  lv_style_set_translate_y(&s_exit_button_pressed_style, 2);
  lv_style_set_text_opa(&s_exit_button_pressed_style, LV_OPA_70);

  lv_style_init(&s_title_style);
  lv_style_set_text_color(&s_title_style, lv_color_black());
  lv_style_set_text_font(&s_title_style, &lv_font_montserrat_22);

  lv_style_init(&s_bpm_style);
  lv_style_set_text_color(&s_bpm_style, lv_color_black());
  lv_style_set_text_font(&s_bpm_style, &lv_font_montserrat_22);
  lv_style_set_text_align(&s_bpm_style, LV_TEXT_ALIGN_LEFT);

  lv_style_init(&s_status_style);
  lv_style_set_text_color(&s_status_style, lv_color_hex(0x5D6672));
  lv_style_set_text_font(&s_status_style, &lv_font_montserrat_12);
  lv_style_set_text_align(&s_status_style, LV_TEXT_ALIGN_LEFT);

  s_styles_ready = true;
}

// Top-left exit button (placeholder action for now).
void setup_exit_button(HeartrateUI *ui) {
  ui->exit_button = lv_btn_create(ui->screen);
  lv_obj_set_pos(ui->exit_button, kExitButtonX, kExitButtonY);
  lv_obj_set_size(ui->exit_button, kExitButtonW, kExitButtonH);
  lv_obj_add_style(ui->exit_button, &s_exit_button_style, LV_PART_MAIN);
  lv_obj_add_style(ui->exit_button, &s_exit_button_pressed_style, LV_STATE_PRESSED);
  lv_obj_add_event_cb(ui->exit_button, heartrate_exit_event_handler, LV_EVENT_CLICKED, nullptr);

  ui->exit_label = lv_label_create(ui->exit_button);
  lv_label_set_text(ui->exit_label, LV_SYMBOL_LEFT);
  lv_obj_center(ui->exit_label);
}

// Header labels: title + numeric BPM + status line.
void setup_labels(HeartrateUI *ui) {
  ui->title_label = lv_label_create(ui->screen);
  lv_obj_set_pos(ui->title_label, kTitleLabelX, kTitleLabelY);
  lv_obj_set_size(ui->title_label, kTitleLabelW, kTitleLabelH);
  lv_label_set_text(ui->title_label, "HeartRate:");
  lv_obj_add_style(ui->title_label, &s_title_style, LV_PART_MAIN);

  ui->bpm_label = lv_label_create(ui->screen);
  lv_obj_set_pos(ui->bpm_label, kBpmLabelX, kBpmLabelY);
  lv_obj_set_size(ui->bpm_label, kBpmLabelW, kBpmLabelH);
  lv_label_set_text(ui->bpm_label, "--");
  lv_obj_add_style(ui->bpm_label, &s_bpm_style, LV_PART_MAIN);

  ui->status_label = lv_label_create(ui->screen);
  lv_obj_set_pos(ui->status_label, kStatusLabelX, kStatusLabelY);
  lv_obj_set_size(ui->status_label, kStatusLabelW, kStatusLabelH);
  lv_label_set_text(ui->status_label, "Place finger");
  lv_obj_add_style(ui->status_label, &s_status_style, LV_PART_MAIN);
}

// Heart waveform chart that mirrors the Sketch_21 display flow.
void setup_chart(HeartrateUI *ui) {
  ui->chart = lv_chart_create(ui->screen);
  lv_obj_set_pos(ui->chart, kChartX, kChartY);
  lv_obj_set_size(ui->chart, kChartW, kChartH);
  lv_obj_add_style(ui->chart, &s_chart_style, LV_PART_MAIN);

  lv_chart_set_type(ui->chart, LV_CHART_TYPE_LINE);
  lv_chart_set_update_mode(ui->chart, LV_CHART_UPDATE_MODE_SHIFT);
  lv_chart_set_point_count(ui->chart, CHART_POINT_COUNT);
  lv_chart_set_div_line_count(ui->chart, 10, 6);
  lv_obj_set_style_size(ui->chart, 0, LV_PART_INDICATOR);
  lv_obj_set_style_pad_left(ui->chart, 8, LV_PART_MAIN);
  lv_obj_set_style_pad_right(ui->chart, 8, LV_PART_MAIN);
  lv_obj_set_style_pad_top(ui->chart, 8, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(ui->chart, 8, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui->chart, 0, LV_PART_MAIN);
  lv_obj_set_style_line_color(ui->chart, lv_color_hex(0xD8DEE6), LV_PART_MAIN);
  lv_obj_set_style_line_width(ui->chart, 1, LV_PART_MAIN);

  lv_chart_set_axis_tick(ui->chart, LV_CHART_AXIS_PRIMARY_Y, 3, 3, 11, 1, true, 42);
  lv_chart_set_axis_tick(ui->chart, LV_CHART_AXIS_PRIMARY_X, 0, 0, 0, 0, false, 0);
  lv_chart_set_range(ui->chart, LV_CHART_AXIS_PRIMARY_Y, CHART_LOW_LIMIT, CHART_HIGH_LIMIT);

  s_chart_series = lv_chart_add_series(ui->chart, lv_color_hex(0xE55454), LV_CHART_AXIS_PRIMARY_Y);
  lv_chart_set_all_value(ui->chart, s_chart_series, kChartMidpoint);
}

// Initialize MAX30102 and move it to low power until the task starts.
void init_sensor(void) {
  Serial.println("Initializing MAX30102...");
  Wire.begin(HEARTRATE_I2C_SDA, HEARTRATE_I2C_SCL);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    s_sensor_ready = false;
    update_status(HeartrateStatus::SensorError);
    Serial.println("MAX30102/MAX30105 was not found. Please check wiring and power.");
    return;
  }

  particleSensor.setup();
  heartrate_shutdown();

  reset_measurement_state();
  seed_wave_history(0);
  reset_chart();

  s_sensor_ready = true;
  update_status(HeartrateStatus::PlaceFinger);
  Serial.println("MAX30102 ready.");
}

// Put sensor into low power mode.
void heartrate_shutdown(void) {
  particleSensor.shutDown();
}

// Wake sensor before starting continuous sampling.
void heartrate_wake_up(void) {
  particleSensor.wakeUp();
}

// Create the background sampling task once.
void create_heartrate_task(void) {
  if (s_heartrate_task_running || !s_sensor_ready) {
    return;
  }

  s_heartrate_task_running = true;
  xTaskCreate(
    heartrate_task_loop,
    "loopTask_heartrate",
    8192,
    nullptr,
    1,
    &s_heartrate_task_handle);
}

void queue_chart_reset(void) {
  s_chart_reset_pending = true;
}

void queue_chart_value(lv_coord_t value) {
  s_pending_chart_value = value;
  s_chart_value_pending = true;
}

void apply_pending_chart_updates(void) {
  if (s_chart_reset_pending) {
    s_chart_reset_pending = false;
    reset_chart();
  }

  if (!s_chart_value_pending) {
    return;
  }

  s_chart_value_pending = false;
  if (s_chart_series != nullptr && g_heartrate_ui.chart != nullptr) {
    lv_chart_set_next_value(g_heartrate_ui.chart, s_chart_series, s_pending_chart_value);
  }
}

// Moving average baseline used to center the waveform.
long average_ir_window(void) {
  long average = 0;
  for (uint8_t i = 0; i < kWaveAverageSize; ++i) {
    average += s_ir_values[i];
  }
  average /= kWaveAverageSize;
  return average;
}

// Fill the IR history window with one value to avoid startup spikes.
void seed_wave_history(long ir_value) {
  for (uint8_t i = 0; i < kWaveAverageSize; ++i) {
    s_ir_values[i] = ir_value;
  }
  s_ir_spot = 0;
}

// Reset chart data to a centered baseline line.
void reset_chart(void) {
  if (s_chart_series != nullptr && g_heartrate_ui.chart != nullptr) {
    lv_chart_set_all_value(g_heartrate_ui.chart, s_chart_series, kChartMidpoint);
  }
}

// Clear beat-history state when finger is removed or a new session starts.
void reset_measurement_state(void) {
  s_beats_per_minute = 0.0f;
  s_beat_average = 0;
  s_rate_spot = 0;
  s_last_beat_ms = 0;
  memset(s_rates, 0, sizeof(s_rates));
}

// Status is updated in the task, rendered in the UI loop.
void update_status(HeartrateStatus status) {
  s_status = status;
}

const char *status_text(HeartrateStatus status) {
  switch (status) {
    case HeartrateStatus::SensorError:
      return "Sensor error";
    case HeartrateStatus::PlaceFinger:
      return "Place finger";
    case HeartrateStatus::DetectingPulse:
      return "Detecting pulse...";
    case HeartrateStatus::Measuring:
      return "Measuring";
    default:
      return "";
  }
}

// Render latest status/BPM values to labels.
void refresh_ui(void) {
  if (g_heartrate_ui.status_label != nullptr) {
    lv_label_set_text(g_heartrate_ui.status_label, status_text(s_status));
  }

  if (g_heartrate_ui.bpm_label == nullptr) {
    return;
  }

  if (s_status == HeartrateStatus::SensorError ||
      s_status == HeartrateStatus::PlaceFinger ||
      s_beat_average == 0) {
    lv_label_set_text(g_heartrate_ui.bpm_label, "--");
  } else {
    lv_label_set_text_fmt(g_heartrate_ui.bpm_label, "%d", s_beat_average);
  }
}

// Periodic serial trace to help classroom debugging.
void print_status(void) {
  if (!s_sensor_ready) {
    Serial.println("status=Sensor error");
    return;
  }

  Serial.print("IR(raw)=");
  Serial.print(s_last_ir_value);
  Serial.print(", status=");
  Serial.println(status_text(s_status));

  if (!s_finger_present || s_beat_average == 0) {
    return;
  }

  Serial.print("BPM=");
  Serial.print(s_beats_per_minute, 1);
  Serial.print(", Avg BPM=");
  Serial.print(s_beat_average);
  Serial.print(", IR(raw)=");
  Serial.println(s_last_ir_value);
}

// Sketch_21-equivalent sampling/beat-detection loop.
void heartrate_task_loop(void *pvParameters) {
  (void)pvParameters;
  Serial.println("Heart rate task started.");

  while (s_heartrate_task_running) {
    const long ir_value = particleSensor.getIR();
    s_last_ir_value = ir_value;

    if (ir_value < kFingerDetectThreshold) {
      if (s_finger_present) {
        s_finger_present = false;
        reset_measurement_state();
        seed_wave_history(0);
        queue_chart_reset();
        update_status(HeartrateStatus::PlaceFinger);
        Serial.println("Finger removed. Waiting for a stable signal...");
      }
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    if (!s_finger_present) {
      s_finger_present = true;
      reset_measurement_state();
      seed_wave_history(ir_value);
      queue_chart_reset();
      update_status(HeartrateStatus::DetectingPulse);
      Serial.println("Finger detected. Measuring...");
    }

    if (checkForBeat(ir_value)) {
      const long delta = millis() - s_last_beat_ms;
      s_last_beat_ms = millis();
      s_beats_per_minute = 60.0f / (delta / 1000.0f);

      if (s_beats_per_minute < 255.0f && s_beats_per_minute > 50.0f) {
        s_rates[s_rate_spot++] = static_cast<byte>(s_beats_per_minute);
        s_rate_spot %= kRateSize;

        int beat_avg = 0;
        for (uint8_t i = 0; i < kRateSize; ++i) {
          beat_avg += s_rates[i];
        }
        s_beat_average = beat_avg / kRateSize;
        update_status(HeartrateStatus::Measuring);
      }
    }

    s_ir_values[s_ir_spot++] = ir_value;
    s_ir_spot %= kWaveAverageSize;

    const long average = average_ir_window();
    long show_value = (ir_value - average) * kWaveformGain + kChartMidpoint;
    if (show_value < CHART_LOW_LIMIT) {
      show_value = CHART_LOW_LIMIT;
    } else if (show_value > CHART_HIGH_LIMIT) {
      show_value = CHART_HIGH_LIMIT;
    }

    if (ir_value > kFingerDetectThreshold &&
        s_chart_series != nullptr) {
      queue_chart_value(static_cast<lv_coord_t>(show_value));
    }

    if (s_beat_average == 0) {
      update_status(HeartrateStatus::DetectingPulse);
    }
  }

  s_heartrate_task_running = false;
  heartrate_shutdown();
  s_heartrate_task_handle = nullptr;
  vTaskDelete(nullptr);
}

// Placeholder for future page navigation.
void heartrate_exit_event_handler(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
    return;
  }

  Serial.println("Clicked the exit button. Placeholder action.");
}

}  // namespace

HeartrateUI g_heartrate_ui = {};

// Build the full heart-rate screen and start measurement task.
void hartrate_ui_setup(HeartrateUI *ui) {
  create_styles();

  ui->screen = lv_obj_create(nullptr);
  lv_obj_add_style(ui->screen, &s_screen_style, LV_PART_MAIN);

  setup_exit_button(ui);
  setup_labels(ui);
  setup_chart(ui);

  init_sensor();
  refresh_ui();

  hartrate_ui_start();

  Serial.println("Heart rate UI ready.");
}

// Start or resume sensor sampling after the screen is opened.
void hartrate_ui_start(void) {
  if (!s_sensor_ready) {
    init_sensor();
  }

  if (!s_sensor_ready) {
    refresh_ui();
    return;
  }

  reset_measurement_state();
  seed_wave_history(0);
  queue_chart_reset();
  update_status(HeartrateStatus::PlaceFinger);
  heartrate_wake_up();
  create_heartrate_task();
  refresh_ui();
}

// Stop the background sampling task and put the sensor into low-power mode.
void hartrate_ui_stop(void) {
  if (!s_heartrate_task_running) {
    return;
  }

  s_heartrate_task_running = false;

  // Wait up to 500 ms for the task to self-delete.
  uint32_t timeout_ms = 500;
  while (timeout_ms > 0 && s_heartrate_task_handle != nullptr) {
    vTaskDelay(pdMS_TO_TICKS(10));
    timeout_ms -= 10;
  }

  // Ensure sensor is off even if the task didn't exit cleanly.
  if (s_sensor_ready) {
    heartrate_shutdown();
  }

  Serial.println("Heart rate task stopped.");
}

// UI-side refresh loop; sensor sampling runs in background task.
void hartrate_ui_loop(void) {
  apply_pending_chart_updates();
  refresh_ui();

  if (millis() - s_last_report_ms >= kReportIntervalMs) {
    s_last_report_ms = millis();
    print_status();
  }
}
