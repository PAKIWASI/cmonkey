#include "draw.h"
#include "term_buf.h"


// cursor

void draw_move(term_buf* b, int row, int col) {
    tb_appendf(b, "\033[%d;%dH", row, col);
}

void draw_move_col(term_buf* b, int col) {
    tb_appendf(b, "\033[%dG", col);          // move within current row
}

// TODO: diff cursor shapes?

// SGR attributes

void draw_reset(term_buf* b)   { tb_append(b, "\033[0m");  }
void draw_bold(term_buf* b)    { tb_append(b, "\033[1m");  }
void draw_dim(term_buf* b)     { tb_append(b, "\033[2m");  }
void draw_italic(term_buf* b)  { tb_append(b, "\033[3m");  }
void draw_underline(term_buf* b){ tb_append(b, "\033[4m"); }
void draw_blink(term_buf* b)   { tb_append(b, "\033[5m");  }
void draw_reverse(term_buf* b) { tb_append(b, "\033[7m");  }
void draw_strike(term_buf* b)  { tb_append(b, "\033[9m");  }

// true color (24-bit)

void draw_fg(term_buf* b, rgb c) {
    tb_appendf(b, "\033[38;2;%d;%d;%dm", c.r, c.g, c.b);
}

void draw_bg(term_buf* b, rgb c) {
    tb_appendf(b, "\033[48;2;%d;%d;%dm", c.r, c.g, c.b);
}

/* combined: set both fg and bg in one call (saves a tb_appendf) */
void draw_color(term_buf* b, rgb fg, rgb bg) {
    tb_appendf(b, "\033[38;2;%d;%d;%d;48;2;%d;%d;%dm",
               fg.r, fg.g, fg.b,
               bg.r, bg.g, bg.b);
}

// screen

void draw_clear(term_buf* b) {
    tb_append(b, "\033[2J\033[H");           // erase screen + home cursor
}

void draw_clear_line(term_buf* b) {
    tb_append(b, "\033[2K");                 // erase entire current line
}

void draw_clear_to_eol(term_buf* b) {
    tb_append(b, "\033[K");                  // erase from cursor to end of line
}

void draw_hide_cursor(term_buf* b) { tb_append(b, "\033[?25l"); }
void draw_show_cursor(term_buf* b) { tb_append(b, "\033[?25h"); }

/* save/restore cursor position (useful for overlays) */
void draw_save_cursor(term_buf* b)    { tb_append(b, "\033[s"); }
void draw_restore_cursor(term_buf* b) { tb_append(b, "\033[u"); }

/* enter/exit alternate screen buffer — use this so the terminal
   restores its scrollback when your app exits                    */
void draw_alt_screen_enter(term_buf* b) { tb_append(b, "\033[?1049h"); }
void draw_alt_screen_exit(term_buf* b)  { tb_append(b, "\033[?1049l"); }


