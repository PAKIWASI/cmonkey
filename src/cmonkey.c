#include "cmonkey.h"
#include "draw.h"
#include "timer.h"

#include <asm-generic/ioctls.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>


#define NUM_RAND_WORDS 200
#define FPS            60

static volatile sig_atomic_t resize_flag = 0;
static void                  set_term_dims(cmonkey* cm);
static void                  winch_handler(int sig);
static void                  terminal_register_cleanup(void);

// Store original terminal settings for restoration
static struct termios og_term;


void cmonkey_create(cmonkey* cm, const char* wb_path, const char* theme_path, const char* conf_path)
{
    wordbank_create(&cm->wb, wb_path, NUM_RAND_WORDS);
    CHECK_FATAL(!cm->wb.words | !cm->wb.arena, "wordbank creation failed");

    queue_create_stk(&cm->q, (u64)NUM_RAND_WORDS * 2, sizeof(u32), NULL);

    CHECK_FATAL(!theme_load(&cm->t, theme_path), "theme load failed");
    CHECK_FATAL(!config_load(&cm->c, conf_path), "config load failed");

    set_term_dims(cm);

    tb_create(&cm->tb, cm->rows, cm->cols);

    timer_begin(&cm->timer, FPS);

    cm->x  = 1;
    cm->y  = 1;
    cm->vx = 1;
    cm->vy = 1;

    cm->quit = false;
}

void cmonkey_destroy(cmonkey* cm)
{
    wordbank_destroy(&cm->wb);
    queue_destroy_stk(&cm->q);
    tb_destroy(&cm->tb);
}

void cmonkey_begin(cmonkey* cm)
{
    // set SIGWINCH handler
    struct sigaction sa = {.sa_handler = winch_handler};
    sigaction(SIGWINCH, &sa, NULL);

    terminal_register_cleanup();

    CHECK_WARN_RET(tcgetattr(STDIN_FILENO, &og_term) == -1, , "tcgetattr failed");

    // Set up terminal for raw mode
    struct termios raw = og_term;
    raw.c_lflag &= (tcflag_t) ~(ECHO | ICANON);

    // Apply changes after draining output
    CHECK_WARN_RET(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1, , "setting term attr failed");

    // Switch to the alternate screen buffer so we don't clobber the
    // user's scrollback, then hide the cursor and clear to theme colours.
    tb_append_cstr(&cm->tb, "\033[?1049h"); // enter alternate screen
    tb_append_cstr(&cm->tb, CURSOR_HIDE);   // hide cursor

    draw_clear(&cm->tb, &cm->t); // fill screen with theme bg/fg

    tb_flush(&cm->tb);
}

void cmonkey_end(void)
{
    const char* cleanup = "\033[0m"      // hard attribute reset
                          "\033[?25h"    // show cursor (was ?25h, which is correct)
                          "\033[?1049l"; // leave alternate screen
    write(STDOUT_FILENO, cleanup, strlen(cleanup));

    // Restore the saved terminal settings
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &og_term);
}

void cmonkey_update(cmonkey* cm)
{
    // Handle a pending terminal resize
    if (resize_flag) {
        resize_flag = 0;
        set_term_dims(cm);

        // TODO:
        // if user resizes, it must be smaller than original RIGHT??
        // then no need to realloc or create new
        // but what if it was smaller then made larger?

        // tb_destroy(cm->tb);
        // cm->tb = tb_create(cm->rows, cm->cols);
        // CHECK_FATAL(!cm->tb, "term_buf re-create after resize failed");
    }

    cm->x += cm->vx;
    if (cm->x >= cm->rows || cm->x <= 0) {
        cm->vx *= -1;
    }

    cm->y += cm->vy;
    if (cm->y >= cm->cols || cm->y <= 0) {
        cm->vy *= -1;
    }
}

void cmonkey_draw(cmonkey* cm)
{
    // tb_reset(cm->tb);
    draw_clear(&cm->tb, &cm->t);

    // tb_append_cstr(&cm->tb, "\033[H");
    tb_append_cstr(&cm->tb, cm->t.reset);

    Box box = {cm->x, cm->y, 3, 5};
    draw_box(&cm->tb, box, &cm->t, &cm->c);

    char buf[128] = "";

    snprintf(buf, 128, "FPS: %.2f   Delta: %.4f",
             timer_get_fps(&cm->timer), timer_get_delta(&cm->timer));


    draw_text(&cm->tb, 1, 1, &cm->t, buf);

    tb_flush(&cm->tb);
}


// main loop
void cmonkey_run(cmonkey* cm)
{
    while (!cm->quit) {
        timer_tick(&cm->timer);

        cmonkey_update(cm);
        cmonkey_draw(cm);

        timer_end_frame(&cm->timer);
        timer_sleep(&cm->timer);
    }
}


void set_term_dims(cmonkey* cm)
{
    struct winsize ws;

    // get winsize from stdout
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
        WARN("ioctl winsize call failed");
        return;
    }

    cm->rows = ws.ws_row;
    cm->cols = ws.ws_col;
}

// When the user resizes the terminal, the kernel sends a SIGWINCH signal.
// We set a flag here and handle the resize in cmonkey_update().
void winch_handler(int sig)
{
    (void)sig;
    resize_flag = 1;
}


// Signal handler to ensure cleanup on Ctrl+C, etc.
static void signal_handler(int sig)
{
    // Reset attributes, show cursor, leave alternate screen
    const char* cleanup = "\033[0m\033[?25h\033[?1049l";
    write(STDOUT_FILENO, cleanup, strlen(cleanup));

    // Restore terminal settings
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &og_term);

    // Reraise the signal with default behavior
    signal(sig, SIG_DFL);
    raise(sig);
}

// Register cleanup handlers
static void terminal_register_cleanup(void)
{
    signal(SIGINT, signal_handler);  // Ctrl+C
    signal(SIGTERM, signal_handler); // kill command
    signal(SIGQUIT, signal_handler); // Ctrl+'\'
    signal(SIGWINCH, winch_handler); // terminal resize
}


