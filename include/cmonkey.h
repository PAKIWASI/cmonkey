#ifndef CMONKEY_H
#define CMONKEY_H

#include "timer.h"
#include "wordbank.h"
#include "Queue_single.h"
#include "buffer.h"
#include "config.h"


typedef enum {
    // waiting to start test
    CMONKEY_WAITING     = 0,
    // test undergoing
    CMONKEY_UNDERGOING,
    // test finished, on result screen
    CMONKEY_FINISHED,
} CMONKEY_STATE;

typedef struct {
    float test_time;    // total time for test
    float elapsed_time;
    // idx of words we have gone through
    genVec typed;   // taken from queue, handy for going back to a word
    u32   correct;  // how many correct words typed
    u32   curr_char;// index into the current word we are typing
} cmonkey_test;

typedef struct {
    WordBank        wb;         // source of all words
    Queue           incoming;   // take words from front, add more to back
    term_buf        tb;         // buffer to write ansi to, flushed to terminal
    cmonkey_theme   t;          // user-set theme
    cmonkey_conf    c;          // user settings
    cmonkey_timer   timer;      // global timer for tui
    cmonkey_test    test;       // rep. a single test
    u32             rows;
    u32             cols;
    CMONKEY_STATE   state;
    bool            quit;
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
