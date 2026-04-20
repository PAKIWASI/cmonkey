#ifndef CMONKEY_TERM_BUF_H
#define CMONKEY_TERM_BUF_H

#include "common_single.h"


typedef struct {
    char* data;
    u32   len;
    u32   cap;
} term_buf;


/*
    this should be based on terminal size
    only one term_buf for a program
*/
void tb_init(term_buf* b, u32 cap);

void tb_free(term_buf* b);

void tb_append(term_buf* b, const char* s);

void tb_appendn(term_buf* b, const char* s, u32 n);

void tb_appendf(term_buf* b, const char* fmt, ...);  // printf-style

void tb_flush(term_buf* b);   // single write() to STDOUT_FILENO

void tb_resize(term_buf* b, u64 new_cap);

#endif // CMONKEY_TERM_BUF_H
