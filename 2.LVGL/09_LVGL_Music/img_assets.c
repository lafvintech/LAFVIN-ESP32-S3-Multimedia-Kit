/* Aggregate music icon assets so the sketch can include one stable entry point.
 * The generated image files stay untouched in img/, while the exported symbols
 * are renamed here to avoid collisions with pause(), std::left, and std::right.
 */
#define esc music_esc
#include "img/esc.c"
#undef esc

#define left music_left
#include "img/left.c"
#undef left

#define right music_right
#include "img/right.c"
#undef right

#define pause music_pause
#include "img/pause.c"
#undef pause

#define play music_play
#include "img/play.c"
#undef play

#define stop music_stop
#include "img/stop.c"
#undef stop
