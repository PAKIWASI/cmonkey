#include "draw.h"
#include "buffer.h"
#include "config.h"




void draw_text(term_buf* b, u32 row, u32 col, const cmonkey_theme* t, const char* text)
{
    // move cursor to row,col
    draw_move(b, row, col);

    // draw text bg
    draw_bg(b, t->text_bg);

    // draw text fb
    draw_fg(b, t->text_fg);

    // draw the text
    tb_append_cstr(b, text);

    // reset to theme
    draw_theme_reset(b, t);
}


void draw_text_with_color(term_buf* b, u32 row, u32 col, const char* fg,
                  const cmonkey_theme* t, const char* text)
{
    // move cursor to row,col
    draw_move(b, row, col);

    // draw text bg
    draw_bg(b, t->text_bg);

    // override with explicit fg
    if (fg && fg[0]) { draw_fg(b, fg); }
    // or draw normal
    else { draw_fg(b, t->text_fg); }

    // draw the text
    tb_append_cstr(b, text);

    // reset to theme
    draw_theme_reset(b, t);
}




void draw_box(term_buf* b, u32 row, u32 col, u32 h, u32 w, cmonkey_theme* t, cmonkey_conf* c)
{
    // move
    draw_move(b, row, col);

    draw_fg(b, t->border);

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

    // reset theme only, doesnot clear entire screen with theme
    draw_theme_reset(b, t);
}

void draw_words_in_box(term_buf* b, u32 row, u32 col, u32 h, u32 w,
                       const char** words, u32 num_words, const cmonkey_theme* t)
{
    draw_move(b, row, col);
    draw_bg(b, t->text_bg);
    draw_fg(b, t->text_fg);

    // calculate total length, adding each word's len
    // then see if we need to go to the next line
    for (u32 i = 0; i < num_words; i++) {

    }
}


