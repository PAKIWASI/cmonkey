#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "term_buf.h"
#include "draw.h"
#include "theme.h"

#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>


static struct termios g_orig_termios;

static void term_raw_mode(void)
{
    tcgetattr(STDIN_FILENO, &g_orig_termios);
    struct termios raw = g_orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void term_restore(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
}

static void term_get_size(int *rows, int *cols)
{
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    *rows = ws.ws_row;
    *cols = ws.ws_col;
}


/* Words the user must type */
static const char *WORDS[] = {
    "the", "quick", "brown", "fox", "jumps", "over",
    "the", "lazy", "dog", "and", "then", "runs", "away"
};
#define WORD_COUNT  (int)(sizeof(WORDS) / sizeof(WORDS[0]))

typedef struct {
    /* terminal geometry */
    int rows, cols;

    /* typing state */
    int   word_idx;          /* which word we're on               */
    char  input[64];         /* current keystroke buffer          */
    int   input_len;
    int   correct_words;
    int   wrong_flag;        /* did user make a mistake this word? */

    /* theme */
    theme t;

    int quit;
} app;


static theme make_catppuccin_mocha(void)
{
    theme t = {0};
    t.fg      = rgb_hex(0xCDD6F4);   /* text    */
    t.bg      = rgb_hex(0x1E1E2E);   /* base    */
    t.accent  = rgb_hex(0x89B4FA);   /* blue    */
    t.correct = rgb_hex(0xA6E3A1);   /* green   */
    t.wrong   = rgb_hex(0xF38BA8);   /* red     */
    t.dim     = rgb_hex(0x585B70);   /* surface2 (ghost text) */
    t.border_style = BORDER_ROUNDED;
    return t;
}


static void draw_box(term_buf *b, int row, int col,
                     int h, int w, border_style style, rgb color)
{
    const char** bc = BORDER_CHARS[style];

    draw_fg(b, color);

    // top edge
    draw_move(b, row, col);
    tb_append(b, bc[0]);
    for (int i = 0; i < w - 2; i++) { tb_append(b, bc[5]); }
    tb_append(b, bc[1]);

    // sides
    for (int r = 1; r < h - 1; r++) {
        draw_move(b, row + r, col);         tb_append(b, bc[4]);
        draw_move(b, row + r, col + w - 1); tb_append(b, bc[4]);
    }

    // bottom edge
    draw_move(b, row + h - 1, col);
    tb_append(b, bc[2]);
    for (int i = 0; i < w - 2; i++) { tb_append(b, bc[5]); }
    tb_append(b, bc[3]);

    draw_reset(b);
}

// a string horizontally, return starting column (1-indexed)
static int center_col(int cols, int str_len)
{
    int col = (cols - str_len) / 2;
    return col < 1 ? 1 : col + 1;   /* +1: ANSI columns are 1-indexed */
}


// [move to position] → [set colors/style] → [write text] → [reset]
static void render(term_buf* b, const app* a)
{
    draw_clear(b);

    // title bar
    const char* title = "  cmonkey  ";
    draw_move(b, 1, center_col(a->cols, (int)strlen(title)));
    draw_bold(b);
    draw_fg(b, a->t.accent);
    tb_append(b, title);
    draw_reset(b);

    // word display box
    int box_w  = a->cols < 80 ? a->cols - 4 : 76;
    int box_h  = 5;
    int box_r  = (a->rows / 2) - 3;
    int box_c  = center_col(a->cols, box_w);

    draw_box(b, box_r, box_c, box_h, box_w,
             a->t.border_style, a->t.accent);

    // words inside the box — one line, centered
    // Build the display string: [done] [current] [upcoming...]
    char line[512] = {0};
    int  pos = 0;
    for (int i = 0; i < WORD_COUNT && pos < (int)sizeof(line) - 2; i++) {
        if (i > 0) { line[pos++] = ' '; }
        const char* w = WORDS[i];
        size_t wl = strlen(w);
        memcpy(line + pos, w, wl);
        pos += (int)wl;
    }
    line[pos] = '\0';

    int text_row = box_r + 2;

    /* dim: all upcoming words */
    draw_move(b, text_row, center_col(a->cols, pos));
    draw_fg(b, a->t.dim);
    tb_append(b, line);

    /* re-draw done words in correct color */
    int cursor = center_col(a->cols, pos);
    for (int i = 0; i < a->word_idx && i < WORD_COUNT; i++) {
        draw_move(b, text_row, cursor);
        draw_fg(b, a->t.correct);
        tb_append(b, WORDS[i]);
        cursor += (int)strlen(WORDS[i]) + 1;
    }

    /* re-draw current word: match typed chars */
    if (a->word_idx < WORD_COUNT) {
        const char *cur = WORDS[a->word_idx];
        int cur_len = (int)strlen(cur);
        draw_move(b, text_row, cursor);

        for (int c = 0; c < cur_len; c++) {
            if (c < a->input_len) {
                rgb col = (a->input[c] == cur[c]) ? a->t.correct : a->t.wrong;
                draw_fg(b, col);
            } else {
                draw_fg(b, a->t.fg);   /* untyped portion */
            }
            char ch[2] = { cur[c], '\0' };
            tb_append(b, ch);
        }
    }
    draw_reset(b);

    /* ── input prompt ───────────────────────────────────────────────────── */
    int prompt_row = box_r + box_h + 1;
    int prompt_col = center_col(a->cols, 40);

    draw_move(b, prompt_row, prompt_col);
    draw_fg(b, a->t.dim);
    tb_append(b, "> ");
    draw_fg(b, a->wrong_flag ? a->t.wrong : a->t.fg);
    tb_appendn(b, a->input, (u32)a->input_len);
    draw_reset(b);

    /* ── status bar ─────────────────────────────────────────────────────── */
    char status[128];
    int slen = snprintf(status, sizeof(status),
                        " word %d/%d  |  correct: %d  |  q to quit ",
                        a->word_idx + 1, WORD_COUNT, a->correct_words);

    draw_move(b, a->rows, center_col(a->cols, slen));
    draw_dim(b);
    draw_fg(b, a->t.accent);
    tb_append(b, status);
    draw_reset(b);

    /* ── finished screen overlay ────────────────────────────────────────── */
    if (a->word_idx >= WORD_COUNT) {
        const char *done = "  All done! Press q to quit.  ";
        draw_move(b, a->rows / 2 + 1,
                  center_col(a->cols, (int)strlen(done)));
        draw_bold(b);
        draw_fg(b, a->t.correct);
        tb_append(b, done);
        draw_reset(b);
    }
}

/* ══════════════════════════════════════════════════════════════════════════ */
/*  Input handling                                                            */
/* ══════════════════════════════════════════════════════════════════════════ */

static void handle_key(app *a, char ch)
{
    if (ch == 'q' || ch == 3 /* Ctrl-C */) {
        a->quit = 1;
        return;
    }

    if (a->word_idx >= WORD_COUNT) return;   /* already finished */

    /* backspace */
    if (ch == 127 || ch == 8) {
        if (a->input_len > 0) {
            a->input[--a->input_len] = '\0';
            /* recheck wrong flag */
            const char *cur = WORDS[a->word_idx];
            a->wrong_flag = 0;
            for (int i = 0; i < a->input_len; i++) {
                if (a->input[i] != cur[i]) { a->wrong_flag = 1; break; }
            }
        }
        return;
    }

    /* space = submit word */
    if (ch == ' ') {
        const char *cur = WORDS[a->word_idx];
        /* check full match */
        if (a->input_len == (int)strlen(cur) &&
            strncmp(a->input, cur, (size_t)a->input_len) == 0) {
            a->correct_words++;
        }
        a->word_idx++;
        a->input_len  = 0;
        a->wrong_flag = 0;
        memset(a->input, 0, sizeof(a->input));
        return;
    }

    /* regular character */
    if (a->input_len < (int)sizeof(a->input) - 1) {
        const char *cur = WORDS[a->word_idx];
        a->input[a->input_len++] = ch;

        /* update wrong flag */
        int idx = a->input_len - 1;
        if (idx >= (int)strlen(cur) || a->input[idx] != cur[idx])
            a->wrong_flag = 1;
        else if (a->wrong_flag) {
            /* recheck whole word */
            a->wrong_flag = 0;
            for (int i = 0; i < a->input_len; i++) {
                if (a->input[i] != cur[i]) { a->wrong_flag = 1; break; }
            }
        }
    }
}

/* ══════════════════════════════════════════════════════════════════════════ */
/*  SIGWINCH — terminal resize                                                */
/* ══════════════════════════════════════════════════════════════════════════ */

static volatile int g_resized = 0;
static void on_resize(int sig) { (void)sig; g_resized = 1; }


int main(void)
{
    term_raw_mode();
    signal(SIGWINCH, on_resize);

    app a = {0};
    a.t = make_catppuccin_mocha();
    term_get_size(&a.rows, &a.cols);

    // buffer: rows * cols * 16 bytes is generous
    term_buf buf;
    tb_init(&buf, (u32)(a.rows * a.cols * 16));

    draw_alt_screen_enter(&buf);
    draw_hide_cursor(&buf);
    tb_flush(&buf);

    // main loop
    while (!a.quit) {

        if (g_resized) {
            g_resized = 0;
            term_get_size(&a.rows, &a.cols);
        }

        render(&buf, &a);
        tb_flush(&buf);

        char ch = 0;
        if (read(STDIN_FILENO, &ch, 1) == 1)
            handle_key(&a, ch);
    }

    /* ── cleanup ────────────────────────────────────────────────────────── */
    draw_alt_screen_exit(&buf);
    draw_show_cursor(&buf);
    tb_flush(&buf);

    tb_free(&buf);
    term_restore();
    return 0;
}
