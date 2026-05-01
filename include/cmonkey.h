#ifndef CMONKEY_H
#define CMONKEY_H

#include "Queue_single.h"
#include "buffer.h"
#include "config.h"
#include "timer.h"
#include "input.h"
#include "wordbank.h"


typedef enum {
    // waiting to start test
    CMONKEY_WAITING = 0,
    // test undergoing
    CMONKEY_UNDERGOING,
    // test finished, on result screen
    CMONKEY_FINISHED,
} CMONKEY_STATE;

typedef enum {
    WORD_PENDING = 0,
    WORD_CORRECT,
    WORD_INCORRECT,
} WORD_STATE;


// NOTE: tagged as 'struct cmonkey_test' so draw.h can forward-declare it
// without pulling in this whole header (avoids circular include).
typedef struct cmonkey_test {
    genVec typed;           // u32 word-vec indices, in order of typing
    genVec word_states;     // WORD_STATE per committed word (parallel to typed[])
    float  elapsed_time;    // seconds since test started
    u32    typed_base;      // index into typed[] of first visible word (scroll)
    u32    curr_word;       // index into typed[] of word being typed right now
    u32    curr_char;       // unused for now — reserved for future cursor logic
    u32    correct;         // committed correct word count
    u32    incorrect;       // committed incorrect word count
    char   curr_typed[128]; // what the user has typed for curr_word so far
    u32    curr_typed_len;
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
    cmonkey_input inputs[32];
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
