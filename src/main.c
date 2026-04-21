#include "config.h"
#include "term_buf.h"
#include "draw.h"
#include "theme.h"
#include "wordbank.h"

#include <stdio.h>
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

static void term_get_size(u32* rows, u32* cols)
{
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    *rows = ws.ws_row;
    *cols = ws.ws_col;
}


//  SIGWINCH — terminal resize

static volatile int g_resized = 0;
static u32 resize_count = 0;
static void on_resize(int sig) { (void)sig; g_resized = 1; }

static void handle_resize(term_buf* buf, u32* rows, u32* cols)
{
    // buffer will grow automatically if len >= cap
    term_get_size(rows, cols);
    resize_count++;
}


#define ENG "english.json"
#define FOLDER_PATH "wordbanks/"
#define CURR_FILE (FOLDER_PATH ENG)

#define SCREEN_PAD_ROW_FACTOR 1
#define SCREEN_PAD_COL_FACTOR 4

#define TEXTBOX_PAD_X 1
#define TEXTBOX_PAD_Y 2


int main(void)
{
    term_raw_mode();
    // TODO: was not working
    signal(SIGWINCH, on_resize);

    term_buf buf;       // rows, cols, size ???
    u32 rows, cols;
    term_get_size(&rows, &cols);
    tb_init(&buf, (rows * cols * 16));

    draw_alt_screen_enter(&buf);
    draw_hide_cursor(&buf);
    tb_flush(&buf);

    WordBank* wb = wordbank_create(CURR_FILE, 15);
    u32 word_idx[15] = {0};
    wordbank_random_words(wb, word_idx, 15);

    // main loop
    char c = ' ';
    while (c != 'q') {
        // TODO: how to timeout this to 100ms ?
        c = (char)getchar();

        color_role r = { {100, 80, 110}, {0, 0, 0}, 0, false};
        draw_box(&buf, 1, 1, rows, cols, BORDER_BOLD, &r);

        color_role r2 = { {150, 200, 40}, {0, 0, 0}, 0, false};
        draw_box(&buf, rows/4, cols/4, rows/2, cols/2, BORDER_ROUNDED, &r2);

        draw_words(&buf, rows, cols, word_idx, 15, NULL, wb);

        if (g_resized) {
            handle_resize(&buf, &rows, &cols);
            g_resized = 0;
        }

        tb_flush(&buf);
    }

    // cleanup
    draw_alt_screen_exit(&buf);
    draw_show_cursor(&buf);
    tb_flush(&buf);

    wordbank_destroy(wb);
    tb_free(&buf);
    term_restore();

    // DEBUG:

    printf("rows: %d, cols: %d\n", rows, cols);
    printf("resized: %d\n", g_resized);
    printf("resize_count: %d\n", resize_count);
    return 0;
}

