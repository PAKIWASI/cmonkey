#include "draw.h"
#include "buffer.h"
#include "cmonkey.h"
#include "config.h"
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


void draw_text_with_color(term_buf* b, u32 row, u32 col, const char* fg, const cmonkey_theme* t, const char* text)
{
    // move cursor to row,col
    draw_move(b, row, col);

    // draw text bg
    draw_bg(b, t->text_bg);

    // override with explicit fg
    if (fg && fg[0]) {
        draw_fg(b, fg);
    }
    // or draw normal
    else {
        draw_fg(b, t->text_fg);
    }

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

void draw_words_in_box(term_buf* b, Box box, Queue* q, WordBank* wb, u32 num_words, const cmonkey_theme* t)
{
    draw_bg(b, t->text_bg);
    draw_fg(b, t->text_fg);

    u32 inner_w  = box.w - 2; // subtract borders
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
        line_len += w->len + 1; // +1 for space
    }

    draw_theme_reset(b, t);
}

// Returns which inner line (0-indexed) word at index `word_idx` lands on,
// given layout starting from `base`. Returns -1 if it's above base or
// beyond the visible area.
static int layout_line_of(cmonkey_test* test, WordBank* wb,
                           u32 base, u32 word_idx,
                           u32 inner_w, u32 inner_h)
{
    u32 line     = 0;
    u32 line_len = 0;
    u32 total    = (u32)test->typed.size;

    for (u32 i = base; i < total && line < inner_h; i++) {
        u32   vec_idx = *(u32*)genVec_get_ptr(&test->typed, i);
        Word* w       = (Word*)genVec_get_ptr(wb->words, vec_idx);

        if (line_len > 0 && line_len + 1 + w->len > inner_w) {
            line++;
            line_len = 0;
        }

        if (i == word_idx) { return (int)line; }

        line_len += w->len + (line_len > 0 ? 1 : 0);
    }
    return -1;
}

// Advance typed_base by one full line's worth of words.
static void drop_top_line(cmonkey_test* test, WordBank* wb, u32 inner_w)
{
    u32 line_len = 0;
    u32 total    = (u32)test->typed.size;

    for (u32 i = test->typed_base; i < total; i++) {
        u32   vec_idx = *(u32*)genVec_get_ptr(&test->typed, i);
        Word* w       = (Word*)genVec_get_ptr(wb->words, vec_idx);

        if (line_len > 0 && line_len + 1 + w->len > inner_w) {
            // i is the first word of the NEXT line — that's the new base
            test->typed_base = i;
            return;
        }

        line_len += w->len + (line_len > 0 ? 1 : 0);
    }
}

static void scroll_to_curr(cmonkey_test* test, WordBank* wb,
                            u32 inner_w, u32 inner_h)
{
    u32 middle = inner_h / 2;

    // Keep dropping the top line until curr_word is at or above middle
    for (;;) {
        int line = layout_line_of(test, wb, test->typed_base,
                                  test->curr_word, inner_w, inner_h);
        if (line < 0 || (u32)line <= middle) { break; }
        drop_top_line(test, wb, inner_w);
    }
}

void draw_words_in_box_ex(term_buf* b, WordBank* wb, Box box,
                          cmonkey_test* test, const cmonkey_theme* t)
{
    u32 inner_w = box.w - 2; // inside the border
    u32 inner_h = box.h - 2;
 
    // fill the inner area with the text background first so no stale chars show
    draw_bg(b, t->text_bg[0] ? t->text_bg : t->main_bg);
    for (u32 r = 0; r < inner_h; r++) {
        draw_move(b, box.r + 1 + r, box.c + 1);
        for (u32 c2 = 0; c2 < inner_w; c2++) { tb_append_n(b, " ", 1); }
    }
 
    // scroll so curr_word is on the last inner line
    scroll_to_curr(test, wb, inner_w, inner_h);
 
    u32 line     = 0; // 0-indexed inner line
    u32 line_len = 0; // chars used on current line (no leading space on new line)
    u32 total    = (u32)test->typed.size;
 
    for (u32 i = test->typed_base; i < total; i++) {
        if (line >= inner_h) { break; } // out of visible lines
 
        u32   vec_idx = *(u32*)genVec_get_ptr(&test->typed, i);
        Word* w       = (Word*)genVec_get_ptr(wb->words, vec_idx);
        const char* word_str = wordbank_word_at(wb, w->idx);
 
        // word wrapping
        if (line_len > 0 && line_len + 1 + w->len > inner_w) {
            line++;
            line_len = 0;
            if (line >= inner_h) { break; }
        }
 
        u32 draw_col = box.c + 1 + line_len;
        u32 draw_row = box.r + 1 + line;
 
        // add a space between words (not at line start)
        if (line_len > 0) {
            draw_move(b, draw_row, draw_col);
            draw_bg(b, t->text_bg[0] ? t->text_bg : t->main_bg);
            draw_fg(b, t->text_dim[0] ? t->text_dim : t->text_fg);
            tb_append_n(b, " ", 1);
            draw_col++;
            line_len++;
        }
 
        // already committed word
        if (i < test->curr_word) {
            WORD_STATE st = *(WORD_STATE*)genVec_get_ptr(&test->word_states, i);
            const char* fg = (st == WORD_CORRECT) ? t->correct : t->incorrect;
 
            draw_move(b, draw_row, draw_col);
            draw_bg(b, t->text_bg[0] ? t->text_bg : t->main_bg);
            draw_fg(b, fg[0] ? fg : t->text_fg);
            tb_append_n(b, word_str, w->len);
 
            line_len += w->len;
            continue;
        }
 
        // current (active) word
        if (i == test->curr_word) {
            draw_bg(b, t->text_bg[0] ? t->text_bg : t->main_bg);
 
            for (u32 j = 0; j < w->len; j++) {
                draw_move(b, draw_row, draw_col + j);
 
                if (j < test->curr_typed_len) {
                    // typed char: green if correct, red if wrong
                    const char* fg = (test->curr_typed[j] == word_str[j])
                                     ? t->correct : t->incorrect;
                    draw_fg(b, fg[0] ? fg : t->text_fg);
                } else if (j == test->curr_typed_len) {
                    // cursor position: highlight with cursor colour
                    draw_fg(b, t->cursor[0] ? t->cursor : t->text_fg);
                } else {
                    // not yet reached: dimmed
                    draw_fg(b, t->text_dim[0] ? t->text_dim : t->text_fg);
                }
 
                tb_append_n(b, word_str + j, 1);
            }
 
            // if user typed MORE than the word length, show the overflow
            // as a red cursor sitting one past the end
            if (test->curr_typed_len > w->len) {
                draw_move(b, draw_row, draw_col + w->len);
                draw_fg(b, t->incorrect[0] ? t->incorrect : t->text_fg);
                tb_append_n(b, "_", 1); // visible overflow marker
            }
 
            line_len += w->len;
            continue;
        }
 
        // upcoming word
        draw_move(b, draw_row, draw_col);
        draw_bg(b, t->text_bg[0] ? t->text_bg : t->main_bg);
        draw_fg(b, t->text_dim[0] ? t->text_dim : t->text_fg);
        tb_append_n(b, word_str, w->len);
 
        line_len += w->len;
    }
 
    draw_theme_reset(b, t);
}


