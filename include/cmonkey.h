#ifndef CMONKEY_H
#define CMONKEY_H

#include "wordbank.h"
#include "Queue_single.h"
#include "buffer.h"
#include "config.h"


typedef struct {
    WordBank*       wb;
    Queue*          q;
    term_buf*       tb;
    cmonkey_theme*  t;
    cmonkey_conf*   c;
    u32             rows;
    u32             cols;
    bool            quit;
    // bool            resize;
} cmonkey;

void cmonkey_begin(cmonkey* cm, const char* wb_path, const char* theme_path, const char* conf_path);

void cmonekey_end(cmonkey* cm);

void cmonkey_update(cmonkey* cm);

void cmonkey_draw(cmonkey* cm);



#endif // CMONKEY_H
