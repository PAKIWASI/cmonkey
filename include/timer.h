#ifndef CMONKEY_TIMER_H
#define CMONKEY_TIMER_H

#include "common_single.h"
#include <time.h>


typedef struct {
    // timestamps for frame bounds
    struct timespec frame_start;
    struct timespec frame_end;
    // running deadline
    struct timespec next_frame_time; 
    float           target_dt;
    u8              target_fps;
    float           elapsed;
    float           remaining;
} cmonkey_timer;


// Initialize timer once on app start
void timer_begin(cmonkey_timer* timer, u8 target_fps);

// Mark the start of a new frame
void timer_tick(cmonkey_timer* timer);

// Mark the end of frame processing and calculate elapsed time
void timer_end_frame(cmonkey_timer* timer);

// Sleep for remaining time in frame to maintain target FPS
void timer_sleep(cmonkey_timer* timer);

// Get current fps
static inline float timer_get_fps(const cmonkey_timer* t)
{
    return t->elapsed > 0.0f ? 1.0f / t->elapsed : 0.0f;
}

// Get frame delta time in seconds
static inline float timer_get_delta(const cmonkey_timer* t)
{
    return t->elapsed;
}



#endif // CMONKEY_TIMER_H
