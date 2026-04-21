#include "term_buf.h"
#include "draw.h"

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


//  SIGWINCH — terminal resize

static volatile int g_resized = 0;
static void on_resize(int sig) { (void)sig; g_resized = 1; }


int main(void)
{
    term_raw_mode();
    signal(SIGWINCH, on_resize);

    // app a = {0};
    // a.t = make_catppuccin_mocha();
    // term_get_size(&a.rows, &a.cols);

    // buffer: rows * cols * 16 bytes is generous
    term_buf buf;
    // tb_init(&buf, (u32)(a.rows * a.cols * 16));

    draw_alt_screen_enter(&buf);
    draw_hide_cursor(&buf);
    tb_flush(&buf);

    // main loop
    while () {

    }

    // cleanup
    draw_alt_screen_exit(&buf);
    draw_show_cursor(&buf);
    tb_flush(&buf);

    tb_free(&buf);
    term_restore();
    return 0;
}

