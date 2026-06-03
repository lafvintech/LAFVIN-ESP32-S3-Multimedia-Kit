#ifndef __IMG_INDEX_H
#define __IMG_INDEX_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t allinone_img_cam;
extern const lv_img_dsc_t allinone_img_gallery;
extern const lv_img_dsc_t allinone_img_music;
extern const lv_img_dsc_t allinone_img_heartrate;
extern const lv_img_dsc_t allinone_img_rgb;
extern const lv_img_dsc_t allinone_img_ring;
extern const lv_img_dsc_t allinone_img_wifi;
extern const lv_img_dsc_t allinone_img_lafvin;

extern const lv_img_dsc_t allinone_img_esc;
extern const lv_img_dsc_t allinone_img_left;
extern const lv_img_dsc_t allinone_img_right;
extern const lv_img_dsc_t allinone_img_pause;
extern const lv_img_dsc_t allinone_img_play;
extern const lv_img_dsc_t allinone_img_stop;

#ifdef __cplusplus
}
#endif

#define APP_IMG_CAM        (&allinone_img_cam)
#define APP_IMG_GALLERY    (&allinone_img_gallery)
#define APP_IMG_MUSIC      (&allinone_img_music)
#define APP_IMG_HEARTRATE  (&allinone_img_heartrate)
#define APP_IMG_RGB        (&allinone_img_rgb)
#define APP_IMG_RING       (&allinone_img_ring)
#define APP_IMG_WIFI       (&allinone_img_wifi)
#define APP_IMG_LAFVIN     (&allinone_img_lafvin)

#define APP_IMG_ESC        (&allinone_img_esc)
#define APP_IMG_LEFT       (&allinone_img_left)
#define APP_IMG_RIGHT      (&allinone_img_right)
#define APP_IMG_PAUSE      (&allinone_img_pause)
#define APP_IMG_PLAY       (&allinone_img_play)
#define APP_IMG_STOP       (&allinone_img_stop)

// Backward-compatible aliases used by the copied demo modules.
#define GALLERY_IMG_CAM    APP_IMG_CAM
#define GALLERY_IMG_ESC    APP_IMG_ESC
#define GALLERY_IMG_LEFT   APP_IMG_LEFT
#define GALLERY_IMG_RIGHT  APP_IMG_RIGHT

#define MUSIC_IMG_ESC      APP_IMG_ESC
#define MUSIC_IMG_LEFT     APP_IMG_LEFT
#define MUSIC_IMG_RIGHT    APP_IMG_RIGHT
#define MUSIC_IMG_PAUSE    APP_IMG_PAUSE
#define MUSIC_IMG_PLAY     APP_IMG_PLAY
#define MUSIC_IMG_STOP     APP_IMG_STOP

#endif
