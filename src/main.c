#include "cmonkey.h"
#include "term_buf.h"
#include "draw.h"

#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>


// ---- terminal setup ----

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


// ---- SIGWINCH ----

static volatile int g_resized = 0;
static void on_resize(int sig) { (void)sig; g_resized = 1; }

static void handle_resize(cmonkey* cm)
{
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    cm->term.rows = ws.ws_row;
    cm->term.cols = ws.ws_col;
}

// ---- main ----

int main(void)
{
    term_raw_mode();
    signal(SIGWINCH, on_resize);

    cmonkey cm;
    if (!cmonkey_init(&cm, DEFAULT_THEME_PATH, DEFAULT_CONF_PATH, DEFAULT_WORDBANK)) {
        term_restore();
        fprintf(stderr, "cmonkey_init failed\n");
        return 1;
    }

    term_buf buf;   
    // TODO: find best way to init it. why does 16 here works?
    tb_init(&buf, cm.term.rows * cm.term.cols * 16);

    draw_alt_screen_enter(&buf);
    draw_hide_cursor(&buf);
    tb_flush(&buf);

    while (!cm.quit) {
        if (g_resized) {
            handle_resize(&cm);
            g_resized = 0;
        }

        // render(&buf, &cm);
        tb_flush(&buf);

        char c = (char)getchar();
        switch (c) {
            case 'q': cm.quit = true;          break;
            case 'r': cmonkey_new_test(&cm);   break;
            default:  break;
        }
    }

    draw_alt_screen_exit(&buf);
    draw_show_cursor(&buf);
    tb_flush(&buf);

    cmonkey_end(&cm);
    tb_free(&buf);
    term_restore();

    return 0;
}


