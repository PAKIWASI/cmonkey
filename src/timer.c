#include "timer.h"


// time difference as float seconds
#define timespec_diff_s(a, b) \
    ((float)(b.tv_sec - a.tv_sec) + ((float)(b.tv_nsec - a.tv_nsec) / 1e9f))


void timer_begin(cmonkey_timer* t, u8 target_fps)
{
    t->target_fps = target_fps;
    // pre-compute target_dt once so you're not dividing every frame
    t->target_dt = 1.0f / (float)target_fps;
    t->elapsed   = t->target_dt;
    t->remaining = 0.0f;
    clock_gettime(CLOCK_MONOTONIC, &t->frame_start);
    // seeds next_frame_time to right now. This is the running deadline
    // it will be advanced by exactly target_dt each frame
    // first sleep is one frame from the actual start, not from zero
    t->next_frame_time = t->frame_start;
}

void timer_tick(cmonkey_timer* t)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    // full frame time: previous tick to this tick (includes sleep)
    t->elapsed = timespec_diff_s(t->frame_start, now);

    t->frame_start = now;

    // advance deadline
    t->next_frame_time.tv_nsec += (long)(t->target_dt * 1e9f);
    if (t->next_frame_time.tv_nsec >= 1000000000L) {
        t->next_frame_time.tv_sec += 1;
        t->next_frame_time.tv_nsec -= 1000000000L;
    }
}

void timer_end_frame(cmonkey_timer* t)
{
    // just compute remaining for sleep
    clock_gettime(CLOCK_MONOTONIC, &t->frame_end);
    t->remaining = timespec_diff_s(t->frame_end, t->next_frame_time);
    if (t->remaining < 0.0f) {
        t->remaining = 0.0f;
    }
}

void timer_sleep(cmonkey_timer* t)
{
    // sleeps until an absolute monotonic time
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t->next_frame_time, NULL);
}

