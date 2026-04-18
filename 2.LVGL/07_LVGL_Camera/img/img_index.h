#ifndef __IMG_INDEX_H
#define __IMG_INDEX_H

#include "lvgl.h"

// Shared image index for gallery overlay icons.
#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t esc;
extern const lv_img_dsc_t cam;

#ifdef __cplusplus
}
#endif

#define GALLERY_IMG_ESC   (&esc)
#define GALLERY_IMG_CAM   (&cam)

#endif
