#ifndef CMONKEY_H
#define CMONKEY_H

#include "Queue_single.h"
#include "buffer.h"
#include "config.h"
#include "timer.h"
#include "wordbank.h"


typedef enum {
    // waiting to start test
    CMONKEY_WAITING = 0,
    // test undergoing
    CMONKEY_UNDERGOING,
    // test finished, on result screen
    CMONKEY_FINISHED,
} CMONKEY_STATE;

typedef struct {
    float  elapsed_time;
    genVec typed;      // u32 word indices pulled from queue (history + current screen)
    u32    typed_base; // idx of first word currently visible on screen
    u32    curr_word;  // idx into typed[] of the word being typed
    u32    curr_char;  // byte offset into that word
    u32    correct;
    u32    incorrect;
} cmonkey_test;

typedef struct {
    WordBank      wb;       // source of all words
    Queue         incoming; // take words from front, add more to back
    term_buf      tb;       // buffer to write ansi to, flushed to terminal
    cmonkey_theme t;        // user-set theme
    cmonkey_conf  c;        // user settings
    cmonkey_timer timer;    // global timer for tui
    cmonkey_test  test;     // represents a single test
    u32           rows;
    u32           cols;
    CMONKEY_STATE state;
    float         test_time; // current total test time
    bool          quit;
} cmonkey;


// create the struct
void cmonkey_create(cmonkey* cm, const char* wb_path, const char* theme_path, const char* conf_path);

// destroy struct
void cmonkey_destroy(cmonkey* cm);

// setup terminal
void cmonkey_init_term(cmonkey* cm);

// restore terminal etc
void cmonkey_end_term(void);

// per frame logic change based on user input, time etc
void cmonkey_update(cmonkey* cm);

// drawing logic
void cmonkey_draw(cmonkey* cm);

// frame-capped game loop
void cmonkey_run(cmonkey* cm);


// test stuff

void cmonkey_test_new(cmonkey* cm);


#endif // CMONKEY_H
