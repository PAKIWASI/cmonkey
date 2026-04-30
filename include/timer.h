#ifndef CMONKEY_TIMER_H
#define CMONKEY_TIMER_H

#include "common_single.h"
#include <time.h>


typedef struct {
    struct timespec frame_start;
    struct timespec frame_end;
    float elapsed;
    float remaining;
    u8 target_fps;
} cmonkey_timer;


// timer started once on app start
cmonkey_timer* timer_begin(u8 target_fps);

// tick the timer once per frame
void timer_tick(cmonkey_timer* timer);

// sleep for remaining time in frame
void timer_sleep(cmonkey_timer* timer);



#endif // CMONKEY_TIMER_H
