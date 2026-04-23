#ifndef CMONKEY_DRAW
#define CMONKEY_DRAW

#include "buffer.h"

// escape seq
// ==============

// base
#define ESC "\033["

// text
#define BOLD_ON       ESC "1m"
#define BOLD_OFF      ESC "22m"
#define DIM_ON        ESC "2m"
#define DIM_OFF       ESC "22m"      // same as bold off
#define ITALIC_ON     ESC "3m"
#define ITALIC_OFF    ESC "23m"
#define UNDERLINE_ON  ESC "4m"
#define UNDERLINE_OFF ESC "24m"
#define BLINK_ON      ESC "5m"
#define BLINK_OFF     ESC "25m"
#define STRIKE_ON     ESC "9m"
#define STRIKE_OFF    ESC "29m"


static inline void draw_move(term_buf* b, u32 row, u32 col)
{
    tb_append_v(b, ESC "%d;%dH", row, col);
}

// TODO: should this take theme struct?
// clear the screen, apply theme's bg and fg
// this is applied per logical block
static inline void draw_clear(term_buf* b, char* reset)
{
    tb_append_cstr(b, "\033[0m");
}

// text attributes 
static inline void draw_bold_on(term_buf* b)      { tb_append_cstr(b, BOLD_ON); }
static inline void draw_bold_off(term_buf* b)     { tb_append_cstr(b, BOLD_OFF); }
static inline void draw_italic_on(term_buf* b)    { tb_append_cstr(b, ITALIC_ON); }
static inline void draw_italic_off(term_buf* b)   { tb_append_cstr(b, ITALIC_OFF); }
static inline void draw_underline_on(term_buf* b) { tb_append_cstr(b, UNDERLINE_ON); }
static inline void draw_underline_off(term_buf* b){ tb_append_cstr(b, UNDERLINE_OFF); }
// … dim, blink, strike …


// theme colours 
// pass the exact escape string directly
// TODO: this should take theme struct?
static inline void draw_fg(term_buf* b, const char *escape) { tb_append_cstr(b, escape); }
static inline void draw_bg(term_buf* b, const char *escape) { tb_append_cstr(b, escape); }


// full draw functions for elements
static inline void draw_box(term_buf* b, u32 w, u32 h)
{

    // TODO: reset at the end
}


#endif // CMONKEY_DRAW
