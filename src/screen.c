#include "screen.h"
#include "draw.h"
#include <string.h>


void screen_flush(screen* s, term_buf* b, const cmonkey_theme* t)
{
    const char* last_fg = NULL;
    const char* last_bg = NULL;
    bool        pending_move = true;  // force move at first diff

    for (u32 r = 0; r < s->rows; r++) {
        for (u32 c = 0; c < s->cols; c++) {
            cell* back  = &s->back [(r * s->cols) + c];
            cell* front = &s->front[(r * s->cols) + c];

            // skip if identical
            // TODO: 
            if (memcmp(back, front, sizeof(cell)) == 0) {
                pending_move = true;  // next write needs a move
                continue;
            }

            // only emit move when we skipped at least one cell
            if (pending_move) {
                draw_move(b, r + 1, c + 1);  // terminal is 1-indexed
                pending_move = false;
            }

            // only emit color escapes when they actually change
            if (!last_bg || strcmp(back->bg, last_bg) != 0) {
                tb_append_cstr(b, back->bg);
                last_bg = back->bg;
            }
            if (!last_fg || strcmp(back->fg, last_fg) != 0) {
                tb_append_cstr(b, back->fg);
                last_fg = back->fg;
            }

            tb_append_n(b, back->ch, strlen(back->ch));

            // copy cell to front
            *front = *back;
        }
    }
}


