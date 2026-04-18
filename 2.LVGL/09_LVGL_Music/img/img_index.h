#ifndef __IMG_INDEX_H
#define __IMG_INDEX_H

#include "lvgl.h"

// Shared image index for music overlay icons.
#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t music_esc;
extern const lv_img_dsc_t music_left;
extern const lv_img_dsc_t music_right;
extern const lv_img_dsc_t music_pause;
extern const lv_img_dsc_t music_play;
extern const lv_img_dsc_t music_stop;

#ifdef __cplusplus
}
#endif

#define MUSIC_IMG_ESC   (&music_esc)
#define MUSIC_IMG_LEFT  (&music_left)
#define MUSIC_IMG_RIGHT (&music_right)
#define MUSIC_IMG_PAUSE (&music_pause)
#define MUSIC_IMG_PLAY  (&music_play)
#define MUSIC_IMG_STOP  (&music_stop)

#endif
