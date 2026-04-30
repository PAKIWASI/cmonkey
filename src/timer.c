#include "timer.h"


// time difference as float seconds
#define timespec_diff_s(a, b) \
    ((float)(b.tv_sec - a.tv_sec) + ((float)(b.tv_nsec - a.tv_nsec) / 1e9f))


void timer_begin(cmonkey_timer* t, u8 target_fps)
{
    t->target_fps = target_fps;
    // pre-compute target_dt once so you're not dividing every frame
    t->target_dt  = 1.0f / (float)target_fps;
    t->elapsed    = 0.0f;
    t->remaining  = 0.0f;
    clock_gettime(CLOCK_MONOTONIC, &t->frame_start);
    // seeds next_frame_time to right now. This is the running deadline
    // it will be advanced by exactly target_dt each frame
    // first sleep is one frame from the actual start, not from zero
    t->next_frame_time = t->frame_start;
}

// snapshots the current time
// Called at the top of game loop
void timer_tick(cmonkey_timer* t)
{
    clock_gettime(CLOCK_MONOTONIC, &t->frame_start);
}

// Snapshots end time, computes how long the frame's work actually took
void timer_end_frame(cmonkey_timer* t)
{
    clock_gettime(CLOCK_MONOTONIC, &t->frame_end);
    t->elapsed = timespec_diff_s(t->frame_start, t->frame_end);

    // Advance next_frame_time by one target frame
    // manual carry — tv_nsec must stay in [0, 999999999]
    // If adding the frame duration pushes it over one billion nanoseconds
    // you carry 1 into tv_sec and subtract
    t->next_frame_time.tv_nsec += (long)(t->target_dt * 1e9f);
    if (t->next_frame_time.tv_nsec >= 1000000000L) {
        t->next_frame_time.tv_sec += 1;
        t->next_frame_time.tv_nsec -= 1000000000L;
    }

    // Remaining = how long until next frame should start
    t->remaining = timespec_diff_s(t->frame_end, t->next_frame_time);
    if (t->remaining < 0.0f) {
        t->remaining = 0.0f; // overran — don't sleep
    }
}

void timer_sleep(cmonkey_timer* t)
{
    // sleeps until an absolute monotonic time
    clock_nanosleep(
        CLOCK_MONOTONIC,
        TIMER_ABSTIME,
        &t->next_frame_time,
        NULL
    );
}



