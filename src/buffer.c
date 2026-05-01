#include "buffer.h"
#include "common_single.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


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
    if (b) {
        if (b->data) {
            free(b->data);
        }
    }
}

void tb_reset(term_buf* b)
{
    b->len = 0;
}

void tb_append_cstr(term_buf* b, const char* s)
{
    u32 n = (u32)strlen(s);
    CHECK_WARN_RET(n + b->len >= b->cap, , "buf not enough");

    strncpy(NEXT_SLOT(b), s, n);
    b->len += n;
}

void tb_append_n(term_buf* b, const char* s, u32 n)
{
    CHECK_WARN_RET(n + b->len >= b->cap, , "buf not enough");

    strncpy(NEXT_SLOT(b), s, n);
    b->len += n;
}

void tb_append_v(term_buf* b, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    // vsnprintf into b->data + b->len, grow if needed
    u32 needed = (u32)vsnprintf(b->data + b->len, b->cap - b->len, fmt, ap);
    CHECK_WARN_RET(b->len + needed >= b->cap, , "buf not enough");
    b->len += needed;
    va_end(ap);
}

void tb_flush(term_buf* b)
{
    write(STDOUT_FILENO, b->data, b->len);
    b->len = 0; // reset each frame
}
