#include "draw.h"
#include "buffer.h"
#include "config.h"
#include "gen_vector_single.h"
#include "wc_macros_single.h"
#include "wordbank.h"




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


void draw_box(term_buf* b, Box box, cmonkey_theme* t, cmonkey_conf* c)
{
    // move
    draw_move(b, box.r, box.c);

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
    for (u32 i = 0; i < box.w - 2; i++) {
        tb_append_cstr(b, bc[5]);
    }
    // top right
    tb_append_cstr(b, bc[1]);

    // Vertical sides
    for (u32 i = 0; i < box.h - 2; i++) {
        draw_move(b, box.r + 1 + i, box.c); // left side
        tb_append_cstr(b, bc[4]);
        draw_move(b, box.r + 1 + i, box.c + box.w - 1); // right side
        tb_append_cstr(b, bc[4]);
    }

    // Bottom border
    draw_move(b, box.r + box.h - 1, box.c);
    tb_append_cstr(b, bc[2]); // bottom-left
    for (u32 i = 0; i < box.w - 2; i++) {
        tb_append_cstr(b, bc[5]);
    }
    tb_append_cstr(b, bc[3]); // bottom-right

    // reset theme only, doesnot clear entire screen with theme
    draw_theme_reset(b, t);
}

void draw_words_in_box(term_buf* b, Box box, Queue* q, WordBank* wb,
                       u32 num_words, const cmonkey_theme* t)
{
    draw_bg(b, t->text_bg);
    draw_fg(b, t->text_fg);

    u32 inner_w = box.w - 2;  // subtract borders
    u32 line     = box.r + 1;
    u32 line_len = 1;

    for (u32 i = 0; i < num_words; i++) {
        Word* w = (Word*)genVec_get_ptr(wb->words, DEQUEUE(q, u32));

        // wrap before drawing if word doesn't fit
        if (line_len + w->len >= inner_w) {
            line++;
            line_len = 1;
        }

        draw_move(b, line, box.c + 1 + line_len);
        tb_append_n(b, wordbank_word_at(wb, w->idx), w->len);
        line_len += w->len + 1;  // +1 for space
    }

    draw_theme_reset(b, t);
}


