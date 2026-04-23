#ifndef TIMER_H
#define TIMER_H



typedef struct {
    float total_time;
    float elapsed;
} cmonkey_test_timer;

typedef struct {
    float last_frame;
    float curr_frame;
    float target_frames;    //fps
} cmonkey_fps_timer;



void cmonkey_tick_timer(cmonkey_test_timer* test_timer);


void cmonkey_set_fps(cmonkey_fps_timer* fps_timer);

void cmonkey_tick_fps(cmonkey_fps_timer* fps_timer);

void cmonkey_get_dt(cmonkey_fps_timer* fps_timer);

#endif // TIMER_H
