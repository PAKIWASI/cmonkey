#ifndef CMONKEY_DRAW_H
#define CMONKEY_DRAW_H


#include "term_buf.h"
#include "theme.h"


// cursor
void draw_move(term_buf* b, int row, int col);
void draw_move_col(term_buf* b, int col);

// colors (true color)
void draw_fg(term_buf* b, rgb c);

void draw_bg(term_buf* b, rgb c);

void draw_reset(term_buf* b);

void draw_bold(term_buf* b);


void draw_color(term_buf* b, rgb fg, rgb bg);

void draw_dim(term_buf* b);
void draw_italic(term_buf* b);
void draw_underline(term_buf* b);
void draw_blink(term_buf* b);
void draw_reverse(term_buf* b);
void draw_strike(term_buf* b);


// screen
void draw_clear(term_buf* b);
void draw_clear_line(term_buf* b);
void draw_clear_to_eol(term_buf* b);

void draw_save_cursor(term_buf* b);
void draw_restore_cursor(term_buf* b);

void draw_hide_cursor(term_buf* b);

void draw_show_cursor(term_buf* b);

void draw_alt_screen_enter(term_buf* b);

void draw_alt_screen_exit(term_buf* b);


#endif // CMONKEY_DRAW_H
