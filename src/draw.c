#include "draw.h"
#include "term_buf.h"
#include <string.h>


void draw_box(term_buf* b, u32 row, u32 col,
              u32 h, u32 w, border_style style, const color_role* role)
{
    const char** bc = BORDER_CHARS[style];
    draw_role(b, role);

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
                const char** words, u32 n,
                const color_role* role, u32 max_cols)
{
    draw_role(b, role);

    u32 cur_col = col;
    u32 cur_row = row;

    for (u32 i = 0; i < n; i++) {
        const char* w   = words[i];
        u32         len = (u32)strlen(w);

        // +1 for the trailing space between words
        if (cur_col + len + 1 > col + max_cols) {
            cur_row++;
            cur_col = col;
        }

        draw_move(b, cur_row, cur_col);
        tb_append(b, w);
        tb_append(b, " ");
        cur_col += len + 1;
    }

    draw_reset(b);
}


