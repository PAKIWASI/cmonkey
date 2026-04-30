#ifndef CMONKEY_H
#define CMONKEY_H

#include "timer.h"
#include "wordbank.h"
#include "Queue_single.h"
#include "buffer.h"
#include "config.h"


typedef struct {
    WordBank       wb;
    Queue          q;
    term_buf       tb;
    cmonkey_theme  t;
    cmonkey_conf   c;
    cmonkey_timer  timer;
    u32            rows;
    u32            cols;
    // testing
    u32            x;
    u32            y;
    int            vx;
    int            vy;
    //
    bool           quit;
} cmonkey;


// create the struct
void cmonkey_create(cmonkey* cm, const char* wb_path, const char* theme_path, const char* conf_path);
// destroy struct
void cmonkey_destroy(cmonkey* cm);
// setup terminal, get words, init system
void cmonkey_begin(cmonkey* cm);
// restore terminal etc
void cmonkey_end(void);

// per frame logic change based on user input, time etc
void cmonkey_update(cmonkey* cm);

// drawing logic
void cmonkey_draw(cmonkey* cm);

// frame-capped game loop
void cmonkey_run(cmonkey* cm);


#endif // CMONKEY_H
