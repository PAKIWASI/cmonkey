#include "draw.h"
#include "buffer.h"
#include "config.h"



void draw_text_at(term_buf* b, u32 row, u32 col,
                  const char* fg, const char* text)
{
    draw_move(b, row, col);
    if (fg && fg[0]) { draw_fg(b, fg); }
    tb_append_cstr(b, text);
    tb_append_cstr(b, RESET);
}


void draw_box_at(term_buf* b, u32 row, u32 col, u32 h, u32 w,
                 cmonkey_theme* t, cmonkey_conf* c)
{
    // move and theme
    draw_move(b, row, col);
    if (t && t->border[0]) { draw_fg(b, t->border); }

    // border style
    BORDER_STYLE bs = DEFAULT_BORDER_STYLE;
    if (c && c->border_style) { bs = c->border_style; }
    const char** bc = (const char**)BORDER_CHARS[bs];

    //top
    // top left
    tb_append_cstr(b, bc[0]);
    // top horizontal
    for (u32 i = 0; i < w - 2; i++) {
        tb_append_cstr(b, bc[5]);
    }
    // top right
    tb_append_cstr(b, bc[1]);

    // sides verical
    for (u32 i = 0; i < h - 2; i++) {
        draw_move(b, i, 0); tb_append_cstr(b, bc[4]);   // TODO: 1 or 0 ?
        draw_move(b, i, w); tb_append_cstr(b, bc[4]);
    }

    // bottom
    // bottom left
    draw_move(b, h, 0);
    tb_append_cstr(b, bc[2]);
    // bottom horizontal
    for (u32 i = 0; i < w - 2; i++) {
        tb_append_cstr(b, bc[5]);
    }
    // bottom right
    tb_append_cstr(b, bc[3]);

    // reset
    draw_clear(b, t);
}


