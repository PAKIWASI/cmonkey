#ifndef CMONKEY_DRAW_H
#define CMONKEY_DRAW_H


#include "config.h"
#include "term_buf.h"
#include "theme.h"


#define SPREAD(rgb) rgb.r, rgb.g, rgb.b

// cursor

static inline void draw_move(term_buf* b, u32 row, u32 col)
{
    tb_appendf(b, "\033[%d;%dH", row, col);
}

static inline void draw_move_col(term_buf* b, u32 col)
{
    tb_appendf(b, "\033[%dG", col); // move within current row
}

// SGR attributes

static inline void draw_reset(term_buf* b)
{
    tb_append(b, "\033[0m");
}
static inline void draw_bold(term_buf* b)
{
    tb_append(b, "\033[1m");
}
static inline void draw_dim(term_buf* b)
{
    tb_append(b, "\033[2m");
}
static inline void draw_italic(term_buf* b)
{
    tb_append(b, "\033[3m");
}
static inline void draw_underline(term_buf* b)
{
    tb_append(b, "\033[4m");
}
static inline void draw_blink(term_buf* b)
{
    tb_append(b, "\033[5m");
}
static inline void draw_reverse(term_buf* b)
{
    tb_append(b, "\033[7m");
}
static inline void draw_strike(term_buf* b)
{
    tb_append(b, "\033[9m");
}

// true color (24-bit)

static inline void draw_fg(term_buf* b, rgb c)
{
    tb_appendf(b, "\033[38;2;%d;%d;%dm", SPREAD(c));
}

static inline void draw_bg(term_buf* b, rgb c)
{
    tb_appendf(b, "\033[48;2;%d;%d;%dm", SPREAD(c));
}

/* combined: set both fg and bg in one call (saves a tb_appendf) */
static inline void draw_color(term_buf* b, rgb fg, rgb bg)
{
    tb_appendf(b, "\033[38;2;%d;%d;%d;48;2;%d;%d;%dm", SPREAD(fg), SPREAD(bg));
}

// screen

static inline void draw_clear(term_buf* b)
{
    tb_append(b, "\033[2J\033[H"); // erase screen + home cursor
}

static inline void draw_clear_line(term_buf* b)
{
    tb_append(b, "\033[2K"); // erase entire current line
}

static inline void draw_clear_to_eol(term_buf* b)
{
    tb_append(b, "\033[K"); // erase from cursor to end of line
}

static inline void draw_hide_cursor(term_buf* b)
{
    tb_append(b, "\033[?25l");
}
static inline void draw_show_cursor(term_buf* b)
{
    tb_append(b, "\033[?25h");
}

/* save/restore cursor position (useful for overlays) */
static inline void draw_save_cursor(term_buf* b)
{
    tb_append(b, "\033[s");
}
static inline void draw_restore_cursor(term_buf* b)
{
    tb_append(b, "\033[u");
}

/*
    enter/exit alternate screen buffer — use this so the terminal
    restores its scrollback when your app exits
*/
static inline void draw_alt_screen_enter(term_buf* b)
{
    tb_append(b, "\033[?1049h");
}
static inline void draw_alt_screen_exit(term_buf* b)
{
    tb_append(b, "\033[?1049l");
}


// a string horizontally, return starting column (1-indexed)
static inline u32 center_col(u32 cols, u32 str_len)
{
    u32 col = (cols - str_len) / 2;
    return col < 1 ? 1 : col + 1; // +1: ANSI columns are 1-indexed
}

static inline void draw_role(term_buf* b, const color_role* r)
{
    draw_fg(b, r->fg);
    if (r->has_bg) {
        draw_bg(b, r->bg);
    }
    if (r->attrs & ATTR_BOLD) {
        draw_bold(b);
    }
    if (r->attrs & ATTR_ITALIC) {
        draw_italic(b);
    }
    if (r->attrs & ATTR_UNDERLINE) {
        draw_underline(b);
    }
    if (r->attrs & ATTR_DIM) {
        draw_dim(b);
    }
    if (r->attrs & ATTR_STRIKE) {
        draw_strike(b);
    }
}

// common shit to draw

void draw_box(term_buf* b, u32 row, u32 col,
              u32 h, u32 w, border_style style, const color_role* role);

// TODO:
void draw_words(term_buf* b, u32 row, u32 col, const char** words, cmonkey_theme* t);


#endif // CMONKEY_DRAW_H
