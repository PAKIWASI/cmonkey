#include "cmonkey.h"
#include "Queue_single.h"
#include "buffer.h"
#include "draw.h"
#include "input.h"
#include "timer.h"
#include "wordbank.h"

#include <asm-generic/ioctls.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>


static volatile sig_atomic_t resize_flag = 0;

static void set_term_dims(cmonkey* cm);
static void winch_handler(int sig);
static void signal_handler(int sig);
static void terminal_register_cleanup(void);
static void test_refill_words(cmonkey* cm);
static void cmonkey_handle_input(cmonkey* cm, cmonkey_input input);

static struct termios og_term;


#define FPS            60
#define NUM_RAND_WORDS 200
#define DEFAULT_TIME   60.f
#define WORDS_AHEAD    40
// TODO:
#define WORD_SPACING 2
#define LINE_SPACING 2



void cmonkey_create(cmonkey* cm, const char* wb_path, const char* theme_path, const char* conf_path)
{
    wordbank_create(&cm->wb, wb_path, NUM_RAND_WORDS);
    CHECK_FATAL(!cm->wb.words || !cm->wb.arena, "wordbank creation failed");

    queue_create_stk(&cm->incoming, (u64)NUM_RAND_WORDS * 2, sizeof(u32), NULL);

    CHECK_FATAL(!theme_load(&cm->t, theme_path), "theme load failed");
    CHECK_FATAL(!config_load(&cm->c, conf_path), "config load failed");

    set_term_dims(cm);
    tb_create(&cm->tb, cm->rows, cm->cols);
    timer_begin(&cm->timer, FPS);

    cm->state     = CMONKEY_WAITING;
    cm->test_time = DEFAULT_TIME;
    cm->quit      = false;

    wordbank_random_words_in_queue(&cm->wb, &cm->incoming);
    cmonkey_test_new(cm);
}

void cmonkey_destroy(cmonkey* cm)
{
    wordbank_destroy(&cm->wb);
    queue_destroy_stk(&cm->incoming);
    tb_destroy(&cm->tb);
    genVec_destroy_stk(&cm->test.words);
}

void cmonkey_init_term(cmonkey* cm)
{
    // set callback to trigger resize flag
    struct sigaction sa = {.sa_handler = winch_handler};
    sigaction(SIGWINCH, &sa, NULL);

    terminal_register_cleanup();

    CHECK_WARN_RET(tcgetattr(STDIN_FILENO, &og_term) == -1, , "tcgetattr failed");

    struct termios raw = og_term; // preserve original state

    raw.c_lflag &= (tcflag_t) ~(ECHO | ICANON); // set our own
    CHECK_WARN_RET(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1, , "setting term attr failed");

    // must be after raw mode is applied
    input_init();

    tb_append_cstr(&cm->tb, "\033[?1049h"); // enter alternate screen
    tb_append_cstr(&cm->tb, CURSOR_HIDE);   // we don't use the actual cursor

    draw_clear(&cm->tb, &cm->t); // clear screen with theme
    tb_flush(&cm->tb);           // send the commands
}

void cmonkey_end_term(void)
{
    // show cursor, exit alt screen
    const char* cleanup = "\033[0m\033[?25h\033[?1049l";
    write(STDOUT_FILENO, cleanup, strlen(cleanup));
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &og_term); // set back original state
}

void cmonkey_test_new(cmonkey* cm) {}

// Keep typed[] topped up so there are always WORDS_AHEAD words past curr_word.
static void test_refill_words(cmonkey* cm) {}


static void cmonkey_handle_input(cmonkey* cm, cmonkey_input input)
{
    switch (input.action) {
    case ACTION_CHAR:
        // Handle typed character
        if (cm->state == CMONKEY_UNDERGOING) {
            // ... add char to current word ...
        }
        break;

    case ACTION_BACKSPACE:
        // Handle backspace
        // ... delete last char ...
        break;

    case ACTION_DEL_WORD:
        // Handle Ctrl-W
        // ... delete current word ...
        break;

    case ACTION_RESTART:
        // Handle Tab
        cmonkey_test_new(cm);
        break;

    case ACTION_END:
        cm->quit = true;
        break;

    case ACTION_NONE:
    default:
        break;
    }
}

void cmonkey_update(cmonkey* cm)
{
    // tick elapsed time while test is running
    if (cm->state == CMONKEY_UNDERGOING) {
        cm->test.elapsed_time += cm->timer.elapsed;
        if (cm->test.elapsed_time >= cm->test_time) {
            cm->test.elapsed_time = cm->test_time; // it sometimes showed some milisec above
            cm->state             = CMONKEY_FINISHED;
        }
    }

    // Read ALL pending input into cm->inputs
    cm->input_count = input_read_all(cm->inputs);

    // Process each input
    for (u32 i = 0; i < cm->input_count; i++) {
        cmonkey_handle_input(cm, cm->inputs[i]);
    }

    // handle terminal resize
    if (resize_flag) {
        resize_flag = 0;
        set_term_dims(cm);
        // TODO: tb resize if new dims exceed original allocation ?
    }
}

void cmonkey_draw(cmonkey* cm)
{
    // TODO: need to get double buffering

    // clear to theme each frame so stale chars don't linger
    draw_clear(&cm->tb, &cm->t);

    Box border = {1, 1, cm->rows, cm->cols};
    draw_box(&cm->tb, border, &cm->t, &cm->c);

    switch (cm->state) {
    case CMONKEY_WAITING:
    case CMONKEY_UNDERGOING: {
        Box textbox = {8, 32, 8, (u32)(cm->cols - 64)};
        draw_box(&cm->tb, textbox, &cm->t, &cm->c);
        // draw_words_in_box_ex(&cm->tb, &cm->wb, textbox, &cm->test, &cm->t);
        break;
    }
    case CMONKEY_FINISHED:
        // TODO: result screen
        draw_text(&cm->tb, cm->rows / 2, (cm->cols / 2) - 10, &cm->t, "test complete");
        break;
    }

    Box timebox = {6, 33, 3, 10};
    draw_box(&cm->tb, timebox, &cm->t, &cm->c);
    draw_move(&cm->tb, 7, 34);
    tb_append_v(&cm->tb, "%.2f", cm->test.elapsed_time);

    tb_flush(&cm->tb);
}

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


static void set_term_dims(cmonkey* cm)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
        WARN("ioctl winsize call failed");
        return;
    }
    cm->rows = ws.ws_row;
    cm->cols = ws.ws_col;
}

static void winch_handler(int sig)
{
    (void)sig;
    resize_flag = 1;
}

static void signal_handler(int sig)
{
    const char* cleanup = "\033[0m\033[?25h\033[?1049l";
    write(STDOUT_FILENO, cleanup, strlen(cleanup));
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &og_term);
    signal(sig, SIG_DFL);
    raise(sig);
}

static void terminal_register_cleanup(void)
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGWINCH, winch_handler);
}
