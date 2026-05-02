#ifndef SCREEN_H
#define SCREEN_H

#include "buffer.h"
#include "common_single.h"
#include "config.h"


typedef struct {
    char        ch[4]; // UTF-8 bytes (1-4), null padded, for later mulilangs
    const char* fg;    // points to theme's fg or any fg string
    const char* bg;
    u8          attr; // bold/dim/underline bitmask for later
} cell;

typedef struct {
    // flat arrays simulating matrices
    cell* front; // what's currently on the terminal
    cell* back;  // what we're drawing into this frame
    u32   rows;
    u32   cols;
} screen;


// allocate both buffers, poison front so first frame does a full redraw
void screen_create(screen* s, u32 rows, u32 cols);
void screen_destroy(screen* s);

// clear back buffer to spaces with theme bg/fg — call at start of each frame
void screen_clear(screen* s, const cmonkey_theme* t);

// write a single cell into the back buffer
void screen_set(screen* s, u32 row, u32 col,
                const char* ch, const char* fg, const char* bg);

// diff back vs front, emit only changed cells into term_buf, then swap
void screen_flush(screen* s, term_buf* b, const cmonkey_theme* t);



#endif // SCREEN_H
