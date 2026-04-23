#include "buffer.h"
#include "common_single.h"

#include <stdlib.h>
#include <string.h>


#define NEXT_SLOT(b) (b->data + b->len)


void tb_create(term_buf* b, u32 rows, u32 cols)
{
    /* Allocating buffer size
        - Best case: each cell in rows*cols is a typed char, so cap = rows*cols
        - Worst case: each cell is a color code (20 bytes max)
        draw_move emits 15 bytes per logical block, so we budget rows*32 for moves
        +256 for final safety margin
    */
    u32 cap_bytes = (rows * cols * 16) + 4096;

    b->data = malloc(cap_bytes);
    CHECK_FATAL(!b->data, "data is null");

    b->cap = cap_bytes;
    b->len = 0;
}

void tb_destroy(term_buf* b)
{
    if (b && b->data) {
        free(b->data);
    }
}

void tb_reset(term_buf* b)
{
    b->len = 0;
}

void tb_append_cstr(term_buf* b, const char* s)
{
    u32 n = (u32)strlen(s);
    CHECK_WARN_RET(n + b->len >= b->cap,  , "buf not enough");

    strncpy(NEXT_SLOT(b), s, n);
}

void tb_append_n(term_buf* b, const char* s, u32 n)
{
    CHECK_WARN_RET(n + b->len >= b->cap,  , "buf not enough");

    strncpy(NEXT_SLOT(b), s, n);
}

void tb_append_v(term_buf* b, ...)
{

}
