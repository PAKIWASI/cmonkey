#include "term_buf.h"

#include <stdarg.h>
#include <string.h>
#include <unistd.h>


#define GET_LEN_POS(b) (b->data + b->len)
#define MAYBE_GROW_BUF(b, n)                                 \
    ({                                                       \
        if (__builtin_expect(!!(b->len + n >= b->cap), 0)) { \
            b->cap        = b->len + n + 256;                \
            char* newdata = realloc(b->data, b->cap);        \
            if (newdata) {                                   \
                b->data = newdata;                           \
            }                                                \
        }                                                    \
    })


void tb_init(term_buf* b, u32 cap)
{
    b->data = MALLOC(1, cap, data);
    b->cap  = cap;
    b->len  = 0;
}

void tb_free(term_buf* b)
{
    if (b && b->data) {
        free(b->data);
    }
}

void tb_append(term_buf* b, const char* s)
{
    u32 n = (u32)strlen(s);
    MAYBE_GROW_BUF(b, n);

    strncpy(GET_LEN_POS(b), s, n);
    b->len += n;
}

void tb_appendn(term_buf* b, const char* s, u32 n)
{
    MAYBE_GROW_BUF(b, n);

    strncpy(GET_LEN_POS(b), s, n);
    b->len += n;
}

void tb_appendf(term_buf* b, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    // vsnprintf into b->data + b->len, grow if needed
    u32 needed = (u32)vsnprintf(b->data + b->len, b->cap - b->len, fmt, ap);
    if ((size_t)needed >= b->cap - b->len) {
        b->cap        = b->len + needed + 256;
        char* newdata = realloc(b->data, b->cap);
        if (newdata) {
            b->data = newdata;
        }
        // we're in here meaning upper call to vsnprintf failed due to insufficient size
        va_start(ap, fmt);
        vsnprintf(b->data + b->len, b->cap - b->len, fmt, ap);
    }
    b->len += needed;
    va_end(ap);
}

void tb_flush(term_buf* b)
{
    write(STDOUT_FILENO, b->data, b->len);
    b->len = 0; // reset each frame
}
