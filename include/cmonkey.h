#ifndef CMONKEY_H
#define CMONKEY_H

#include "config.h"
#include "wordbank.h"


typedef struct {
    u32 total_words;
    u32 curr_word;  // index into word arr
    u8  curr_char;  // index into curr_word
    u32 correct;
    float time_left; // start at total time and go to zero
    // TODO: do time here or in timer ? do i even need sepereate timer?
    // i thought i needed it for test timing, smooth fps, cursor timing
} cmonkey_test;


// cmonkey app state

typedef struct {
    cmonkey_theme   theme;
    cmonkey_conf    conf;
    // cmonkey_cursor cursor;
    WordBank        wordbank;

    // cmonkey_timer timer;

    cmonkey_test    curr_test;

    bool            resize;
    bool            quit;
} cmonkey;


bool cmonkey_init(cmonkey* cm);



#endif // CMONKEY_H
