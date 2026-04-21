#include "draw.h"
#include "term_buf.h"



void draw_box(term_buf* b, u32 row, u32 col,
              u32 h, u32 w, border_style style, const color_role* role)
{
    const char** bc = BORDER_CHARS[style];
    draw_role(b, role); // enable user configured colors via role

    // top edge
    draw_move(b, row, col);
    tb_append(b, bc[0]);
    for (u32 i = 0; i < w - 2; i++) { tb_append(b, bc[5]); }
    tb_append(b, bc[1]);

    // sides
    for (u32 r = 1; r < h - 1; r++) {
        draw_move(b, row + r, col);         tb_append(b, bc[4]);
        draw_move(b, row + r, col + w - 1); tb_append(b, bc[4]);
    }

    // bottom edge
    draw_move(b, row + h - 1, col);
    tb_append(b, bc[2]);
    for (u32 i = 0; i < w - 2; i++) { tb_append(b, bc[5]); }
    tb_append(b, bc[3]);

    draw_reset(b);
}

void draw_words(term_buf* b, u32 row, u32 col,
                u32* words, u32 n, cmonkey_theme* t, WordBank* wb)
{
    // draw_role(b, &t->main_text);

    draw_move(b, row, col);

    // TODO: we need some sort of window info here
    // start writing words, if len > remaining space, we move to next line
    for (u32 i = 0; i < n; i++) {
        tb_append(b, wordbank_word_at(wb, words[i]));
    }

    draw_reset(b);
}


