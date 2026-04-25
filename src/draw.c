#include "draw.h"
#include "buffer.h"
#include "config.h"


#define THEME_RESET(b, t)       \
    if (t) {                    \
        draw_theme_reset(b, t); \
    } else {                    \
        draw_reset(b);          \
    }


void draw_text_at(term_buf* b, u32 row, u32 col, const char* fg, const cmonkey_theme* t, const char* text)
{
    draw_move(b, row, col);

    if (fg && fg[0]) {
        draw_fg(b, fg);
    }

    tb_append_cstr(b, text);

    THEME_RESET(b, t);
}


void draw_box_at(term_buf* b, u32 row, u32 col, u32 h, u32 w, cmonkey_theme* t, cmonkey_conf* c)
{
    // move and theme
    draw_move(b, row, col);
    if (t && t->border[0]) {
        draw_fg(b, t->border);
    }

    // border style
    BORDER_STYLE bs = DEFAULT_BORDER_STYLE;
    if (c) {
        bs = c->border_style;
    }
    const char (*bc)[8] = BORDER_CHARS[bs];

    //top
    // top left
    tb_append_cstr(b, bc[0]);
    // top horizontal
    for (u32 i = 0; i < w - 2; i++) {
        tb_append_cstr(b, bc[5]);
    }
    // top right
    tb_append_cstr(b, bc[1]);

    // Vertical sides
    for (u32 i = 0; i < h - 2; i++) {
        draw_move(b, row + 1 + i, col); // left side
        tb_append_cstr(b, bc[4]);
        draw_move(b, row + 1 + i, col + w - 1); // right side
        tb_append_cstr(b, bc[4]);
    }

    // Bottom border
    draw_move(b, row + h - 1, col);
    tb_append_cstr(b, bc[2]); // bottom-left
    for (u32 i = 0; i < w - 2; i++) {
        tb_append_cstr(b, bc[5]);
    }
    tb_append_cstr(b, bc[3]); // bottom-right

    // reset
    THEME_RESET(b, t);
}


