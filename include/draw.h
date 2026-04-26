#ifndef CMONKEY_DRAW
#define CMONKEY_DRAW

#include "Queue_single.h"
#include "buffer.h"
#include "config.h"
#include "wordbank.h"


// escape seq

// base
#define ESC "\033["

#define FG_BEGIN ESC "38;2;"
#define BG_BEGIN ESC "48;2;"

#define RESET ESC "0m"

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

// cursor visibility
#define CURSOR_HIDE   ESC "?25l"
#define CURSOR_SHOW   ESC "?25h"

/*
clear screen + move home
erases all content in the terminal window
The newly revealed area is filled with the currently active background colour
    not necessarily theme’s colour, but whatever attribute is in effect
    at the moment the escape is processed
*/
#define CLEAR_SCREEN  ESC "2J" ESC "H"


// box type with position and dimentions
// row, col, height, widht
typedef struct {
    u32 r, c, h, w;
} Box;


static inline void draw_move(term_buf* b, u32 row, u32 col)
{
    tb_append_v(b, ESC "%d;%dH", row, col);
}

// hard reset: clears all attributes back to terminal default
static inline void draw_reset(term_buf* b)
{
    tb_append_cstr(b, RESET);
}

// theme reset: clears attributes then re-applies theme's main fg+bg
static inline void draw_theme_reset(term_buf* b, const cmonkey_theme* t)
{
    tb_append_cstr(b, t->reset);
}

// clear screen and re-apply theme colours
static inline void draw_clear(term_buf* b, const cmonkey_theme* t)
{
    tb_append_cstr(b, t->reset);        // set theme
    tb_append_cstr(b, CLEAR_SCREEN);    // clear entire screen with theme
}

// text attributes
static inline void draw_bold_on(term_buf* b)      { tb_append_cstr(b, BOLD_ON);       }
static inline void draw_bold_off(term_buf* b)     { tb_append_cstr(b, BOLD_OFF);      }
static inline void draw_dim_on(term_buf* b)       { tb_append_cstr(b, DIM_ON);        }
static inline void draw_dim_off(term_buf* b)      { tb_append_cstr(b, DIM_OFF);       }
static inline void draw_italic_on(term_buf* b)    { tb_append_cstr(b, ITALIC_ON);     }
static inline void draw_italic_off(term_buf* b)   { tb_append_cstr(b, ITALIC_OFF);    }
static inline void draw_underline_on(term_buf* b) { tb_append_cstr(b, UNDERLINE_ON);  }
static inline void draw_underline_off(term_buf* b){ tb_append_cstr(b, UNDERLINE_OFF); }
static inline void draw_strike_on(term_buf* b)    { tb_append_cstr(b, STRIKE_ON);     }
static inline void draw_strike_off(term_buf* b)   { tb_append_cstr(b, STRIKE_OFF);    }

// theme colours: pass the pre-built escape string from cmonkey_theme
static inline void draw_fg(term_buf* b, const char* escape) { tb_append_cstr(b, escape); }
static inline void draw_bg(term_buf* b, const char* escape) { tb_append_cstr(b, escape); }

// TODO: inner for loop is a waste
static inline void fill_box_bg(term_buf* b, Box box, const char* bg)
{
    draw_bg(b, bg);
    for (u32 i = 0; i < box.h; i++) {
        draw_move(b, box.r + i, box.c);
        for (u32 j = 0; j < box.w; j++) {
            tb_append_n(b, " ", 1);
        }
    }
}


// COMPONENTS - Non Trivial elements like boxes, text etc

/*
 * move then write a string with optional fg colour
 * Resets afterwards with theme
 */
void draw_text(term_buf* b, u32 row, u32 col,
                  const cmonkey_theme* t, const char* text);
/*
 * draw text with an explicit color override
*/
void draw_text_with_color(term_buf* b, u32 row, u32 col, const char* fg,
                  const cmonkey_theme* t, const char* text);


/*
 * Draw a box at row, col of size h, w
 * takes optional theme and border stlye
 * otherwise use default
 * resets aftewards with theme
*/
void draw_box(term_buf* b, Box box, cmonkey_theme* t, cmonkey_conf* c);

/*
    knows the box's boundry so warps words to the next line
*/
void draw_words_in_box(term_buf* b, Box box, Queue* q, WordBank* wb,
                       u32 num_words, const cmonkey_theme* t);


#endif // CMONKEY_DRAW
