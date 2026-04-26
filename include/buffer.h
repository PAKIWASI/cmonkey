#ifndef CMONKEY_BUFFER_H
#define CMONKEY_BUFFER_H

#include "common_single.h"


typedef struct {
    char* data;
    u32 len;
    u32 cap;
} term_buf;


term_buf* tb_create(u32 rows, u32 cols);

void tb_destroy(term_buf* b);

void tb_reset(term_buf* b);

void tb_append_cstr(term_buf* b, const char* s);

void tb_append_n(term_buf* b, const char* s, u32 n);

void tb_append_v(term_buf* b, const char* fmt, ...);

void tb_flush(term_buf* b);


#endif // CMONKEY_BUFFER_H
