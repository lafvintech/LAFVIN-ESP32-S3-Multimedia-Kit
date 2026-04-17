#include "music_ui.h"

#include "img/img_index.h"
#include "sd_card.h"

#include "Audio.h"
#include "FS.h"
#include "SD_MMC.h"

MusicUI g_music_ui;

// Audio playback state shared by the music screen.
static Audio s_audio;
static int s_music_index = 1;
static int s_music_count = 0;
static bool s_track_loaded = false;
static bool s_music_paused = false;
static volatile bool s_play_next_pending = false;

// UI layout constants for the 320x240 landscape screen.
static constexpr int kDefaultVolume = 8;
static constexpr lv_coord_t kExitButtonSize = 32;
static constexpr lv_coord_t kExitButtonX = 8;
static constexpr lv_coord_t kExitButtonY = 8;
static constexpr lv_coord_t kSliderX = 60;
static constexpr lv_coord_t kSliderY = 16;
static constexpr lv_coord_t kSliderWidth = 220;
static constexpr lv_coord_t kSliderHeight = 12;
static constexpr lv_coord_t kVolumeLabelX = 120;
static constexpr lv_coord_t kVolumeLabelY = 34;
static constexpr lv_coord_t kVolumeLabelWidth = 100;
static constexpr lv_coord_t kVolumeLabelHeight = 22;
static constexpr lv_coord_t kTitleX = 24;
static constexpr lv_coord_t kTitleY = 92;
static constexpr lv_coord_t kTitleWidth = 240;
static constexpr lv_coord_t kTitleHeight = 48;
static constexpr lv_coord_t kButtonSize = 60;
static constexpr lv_coord_t kButtonY = 164;
static constexpr lv_coord_t kPrevButtonX = 20;
static constexpr lv_coord_t kPlayButtonX = 90;
static constexpr lv_coord_t kStopButtonX = 160;
static constexpr lv_coord_t kNextButtonX = 230;
static constexpr lv_coord_t kButtonPressOffset = 5;

// Show the current volume below the slider.
static void music_ui_update_volume_label(int volume) {
  if (g_music_ui.volume_label == NULL) {
    return;
  }

  lv_label_set_text_fmt(g_music_ui.volume_label, "Vol %d", volume);
}

// Update the centered title. When no track is available, show a fallback message.
static void music_ui_set_title_text(const char *text) {
  if (text != NULL && text[0] != '\0') {
    lv_label_set_text(g_music_ui.title_label, text);
  } else {
    lv_label_set_text(g_music_ui.title_label, "The music folder has no files.");
  }
}

// The middle button displays Pause while playing and Play while stopped/paused.
static void music_ui_update_play_pause_icon() {
  if (g_music_ui.play_pause_button == NULL) {
    return;
  }

  if (!s_track_loaded || s_music_paused) {
    lv_img_set_src(g_music_ui.play_pause_button, MUSIC_IMG_PLAY);
  } else {
    lv_img_set_src(g_music_ui.play_pause_button, MUSIC_IMG_PAUSE);
  }
}

// Disable buttons visually without hiding them from the layout.
static void music_ui_set_button_enabled(lv_obj_t *button, bool enabled) {
  if (button == NULL) {
    return;
  }

  if (enabled) {
    lv_obj_add_flag(button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_img_opa(button, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(button, LV_OPA_TRANSP, LV_PART_MAIN);
  } else {
    lv_obj_clear_flag(button, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_img_opa(button, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_img_recolor(button, lv_color_hex(0x8D96A6), LV_PART_MAIN);
    lv_obj_set_style_img_recolor_opa(button, LV_OPA_40, LV_PART_MAIN);
  }
}

// Refresh button state whenever the file list or playback state changes.
static void music_ui_refresh_controls() {
  const bool has_music = (s_music_count > 0);
  music_ui_set_button_enabled(g_music_ui.prev_button, has_music);
  music_ui_set_button_enabled(g_music_ui.play_pause_button, has_music);
  music_ui_set_button_enabled(g_music_ui.stop_button, has_music);
  music_ui_set_button_enabled(g_music_ui.next_button, has_music);
  music_ui_update_play_pause_icon();
}

// Keep icon buttons visually minimal so only the image is visible.
static void music_ui_button_style_init(lv_obj_t *button) {
  lv_obj_add_flag(button, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_opa(button, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_opa(button, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_shadow_opa(button, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_outline_opa(button, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_pad_all(button, 0, LV_PART_MAIN);
}

// Configure the audio output path used by the ESP32-audioI2S library.
static bool music_ui_audio_init() {
  s_audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  s_audio.setVolume(kDefaultVolume);
  return true;
}

// Stop playback and return the UI to the idle state for the current track.
static void music_ui_stop_current() {
  s_audio.stopSong();
  s_track_loaded = false;
  s_music_paused = true;
  music_ui_refresh_controls();
}

// Load and start a track by linked-list index. The list is 1-based.
static bool music_ui_play_index(int index) {
  if (s_music_count <= 0) {
    s_track_loaded = false;
    s_music_paused = false;
    music_ui_set_title_text(NULL);
    music_ui_refresh_controls();
    return false;
  }

  if (index < 1) {
    index = s_music_count;
  } else if (index > s_music_count) {
    index = 1;
  }

  char *file_name = list_find_node(list_music, index);
  if (file_name == NULL) {
    return false;
  }

  s_audio.stopSong();

  String file_path = String(MUSIC_FOLDER) + "/" + file_name;
  Serial.printf("Loading music: %s\n", file_path.c_str());

  s_audio.connecttoFS(SD_MMC, file_path.c_str());
  s_music_index = index;
  s_track_loaded = true;
  s_music_paused = false;

  music_ui_set_title_text(file_name);
  music_ui_refresh_controls();
  return true;
}

// Prime the screen with the first file name without auto-playing it.
static void music_ui_prepare_first_track() {
  s_music_count = list_count_number(list_music);
  if (s_music_count <= 0) {
    s_music_index = 0;
    s_track_loaded = false;
    s_music_paused = false;
    music_ui_set_title_text(NULL);
    music_ui_refresh_controls();
    return;
  }

  s_music_index = 1;
  s_track_loaded = false;
  s_music_paused = true;
  music_ui_set_title_text(list_find_node(list_music, s_music_index));
  music_ui_refresh_controls();
}

// Move to the previous track with wrap-around support.
static void music_ui_play_previous() {
  if (s_music_count <= 0) {
    return;
  }

  int target = s_music_index > 0 ? s_music_index - 1 : s_music_count;
  music_ui_play_index(target);
}

// Move to the next track with wrap-around support.
static void music_ui_play_next() {
  if (s_music_count <= 0) {
    return;
  }

  int target = s_music_index > 0 ? s_music_index + 1 : 1;
  music_ui_play_index(target);
}

// Start the current track if idle, otherwise toggle pause/resume.
static void music_ui_toggle_play_pause() {
  if (s_music_count <= 0) {
    return;
  }

  if (!s_track_loaded) {
    music_ui_play_index(s_music_index > 0 ? s_music_index : 1);
    return;
  }

  s_audio.pauseResume();
  s_music_paused = !s_music_paused;
  music_ui_refresh_controls();
}

// Placeholder for returning to the chapter's main page later.
static void on_exit_button_clicked(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  Serial.println("Clicked the ESC button. Placeholder action.");
}

// Previous track button callback.
static void on_prev_button_clicked(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  music_ui_play_previous();
}

// Shared play/pause button callback.
static void on_play_pause_button_clicked(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  music_ui_toggle_play_pause();
}

// Next track button callback.
static void on_next_button_clicked(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  music_ui_play_next();
}

// Stop button callback.
static void on_stop_button_clicked(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }

  music_ui_stop_current();
}

// Volume slider callback.
static void on_volume_slider_changed(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) {
    return;
  }

  int volume = lv_slider_get_value(g_music_ui.volume_slider);
  s_audio.setVolume(volume);
  music_ui_update_volume_label(volume);
}

void music_ui_setup(MusicUI *ui) {
  // Create a clean white screen for the music player layout.
  ui->screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(ui->screen, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui->screen, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui->screen, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui->screen, 0, LV_PART_MAIN);

  // Audio and file list must be ready before the first UI refresh.
  music_ui_audio_init();
  setup_list_head_music();

  static lv_style_t pressed_style;
  lv_style_init(&pressed_style);
  lv_style_set_translate_y(&pressed_style, kButtonPressOffset);

  // Top-left exit icon.
  ui->exit_button = lv_img_create(ui->screen);
  lv_obj_set_pos(ui->exit_button, kExitButtonX, kExitButtonY);
  lv_obj_set_size(ui->exit_button, kExitButtonSize, kExitButtonSize);
  lv_img_set_src(ui->exit_button, MUSIC_IMG_ESC);
  music_ui_button_style_init(ui->exit_button);
  lv_obj_add_style(ui->exit_button, &pressed_style, LV_STATE_PRESSED);

  // Center label shows the current file name and scrolls when it is too long.
  ui->title_label = lv_label_create(ui->screen);
  lv_obj_set_pos(ui->title_label, kTitleX, kTitleY);
  lv_obj_set_size(ui->title_label, kTitleWidth, kTitleHeight);
  lv_label_set_long_mode(ui->title_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
  lv_label_set_text(ui->title_label, "");
  lv_obj_set_style_text_align(ui->title_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_text_color(ui->title_label, lv_color_hex(0x111111), LV_PART_MAIN);
  lv_obj_set_style_text_font(ui->title_label, &lv_font_montserrat_20, LV_PART_MAIN);

  // Bottom transport controls: previous, play/pause, stop, next.
  ui->prev_button = lv_img_create(ui->screen);
  lv_obj_set_pos(ui->prev_button, kPrevButtonX, kButtonY);
  lv_obj_set_size(ui->prev_button, kButtonSize, kButtonSize);
  lv_img_set_src(ui->prev_button, MUSIC_IMG_LEFT);
  music_ui_button_style_init(ui->prev_button);
  lv_obj_add_style(ui->prev_button, &pressed_style, LV_STATE_PRESSED);

  ui->play_pause_button = lv_img_create(ui->screen);
  lv_obj_set_pos(ui->play_pause_button, kPlayButtonX, kButtonY);
  lv_obj_set_size(ui->play_pause_button, kButtonSize, kButtonSize);
  music_ui_button_style_init(ui->play_pause_button);
  lv_obj_add_style(ui->play_pause_button, &pressed_style, LV_STATE_PRESSED);

  ui->stop_button = lv_img_create(ui->screen);
  lv_obj_set_pos(ui->stop_button, kStopButtonX, kButtonY);
  lv_obj_set_size(ui->stop_button, kButtonSize, kButtonSize);
  lv_img_set_src(ui->stop_button, MUSIC_IMG_STOP);
  music_ui_button_style_init(ui->stop_button);
  lv_obj_add_style(ui->stop_button, &pressed_style, LV_STATE_PRESSED);

  ui->next_button = lv_img_create(ui->screen);
  lv_obj_set_pos(ui->next_button, kNextButtonX, kButtonY);
  lv_obj_set_size(ui->next_button, kButtonSize, kButtonSize);
  lv_img_set_src(ui->next_button, MUSIC_IMG_RIGHT);
  music_ui_button_style_init(ui->next_button);
  lv_obj_add_style(ui->next_button, &pressed_style, LV_STATE_PRESSED);

  // Top volume slider with a blue theme.
  ui->volume_slider = lv_slider_create(ui->screen);
  lv_obj_set_pos(ui->volume_slider, kSliderX, kSliderY);
  lv_obj_set_size(ui->volume_slider, kSliderWidth, kSliderHeight);
  lv_slider_set_range(ui->volume_slider, 0, 21);
  lv_slider_set_value(ui->volume_slider, kDefaultVolume, LV_ANIM_OFF);
  lv_obj_set_style_bg_color(ui->volume_slider, lv_color_hex(0xB8D9FF), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui->volume_slider, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(ui->volume_slider, lv_color_hex(0x1E5EFF), LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(ui->volume_slider, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(ui->volume_slider, lv_color_hex(0x0F47C8), LV_PART_KNOB);
  lv_obj_set_style_bg_opa(ui->volume_slider, LV_OPA_COVER, LV_PART_KNOB);
  lv_obj_set_style_pad_all(ui->volume_slider, 2, LV_PART_MAIN);

  // Numeric volume readout placed below the slider.
  ui->volume_label = lv_label_create(ui->screen);
  lv_obj_set_pos(ui->volume_label, kVolumeLabelX, kVolumeLabelY);
  lv_obj_set_size(ui->volume_label, kVolumeLabelWidth, kVolumeLabelHeight);
  lv_obj_set_style_text_align(ui->volume_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_text_color(ui->volume_label, lv_color_hex(0x1E5EFF), LV_PART_MAIN);
  lv_label_set_text(ui->volume_label, "");
  music_ui_update_volume_label(kDefaultVolume);

  // Wire up all UI events after every object has been created.
  lv_obj_add_event_cb(ui->exit_button, on_exit_button_clicked, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(ui->prev_button, on_prev_button_clicked, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(ui->play_pause_button, on_play_pause_button_clicked, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(ui->stop_button, on_stop_button_clicked, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(ui->next_button, on_next_button_clicked, LV_EVENT_ALL, NULL);
  lv_obj_add_event_cb(ui->volume_slider, on_volume_slider_changed, LV_EVENT_ALL, NULL);

  // Apply the initial title and button state based on the /music folder.
  music_ui_prepare_first_track();
}

void music_ui_loop(void) {
  // Feed the decoder continuously so playback remains smooth.
  s_audio.loop();

  // Move to the next track outside the audio callback context.
  if (s_play_next_pending) {
    s_play_next_pending = false;
    music_ui_play_next();
  }
}

// Optional ESP32-audioI2S debug callback.
void audio_info(const char *info) {
  Serial.print("info        ");
  Serial.println(info);
}

// Called by the audio library when a local MP3 reaches the end.
void audio_eof_mp3(const char *info) {
  Serial.print("eof_mp3     ");
  Serial.println(info);
  s_play_next_pending = true;
}
