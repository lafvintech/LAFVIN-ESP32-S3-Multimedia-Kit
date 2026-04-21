// Aggregate image assets for the All-in-One sketch and rename generated
// symbols so they can coexist with C/C++ standard names like left/right/pause.

#define cam allinone_img_cam
#include "img/cam.c"
#undef cam

#define gallery allinone_img_gallery
#include "img/gallery.c"
#undef gallery

#define music allinone_img_music
#include "img/music.c"
#undef music

#define heartrate allinone_img_heartrate
#include "img/heartrate.c"
#undef heartrate

#define rgb allinone_img_rgb
#include "img/rgb.c"
#undef rgb

#define ring allinone_img_ring
#include "img/ring.c"
#undef ring

#define wifi allinone_img_wifi
#include "img/wifi.c"
#undef wifi

#define lafvin allinone_img_lafvin
#include "img/lafvin.c"
#undef lafvin

#define esc allinone_img_esc
#include "img/esc.c"
#undef esc

#define left allinone_img_left
#include "img/left.c"
#undef left

#define right allinone_img_right
#include "img/right.c"
#undef right

#define pause allinone_img_pause
#include "img/pause.c"
#undef pause

#define play allinone_img_play
#include "img/play.c"
#undef play

#define stop allinone_img_stop
#include "img/stop.c"
#undef stop
