#include "cmonkey.h"
#include "Queue_single.h"
#include "buffer.h"
#include "draw.h"
#include "input.h"
#include "timer.h"
#include "wc_macros_single.h"
#include "wordbank.h"

#include <asm-generic/ioctls.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>


static volatile sig_atomic_t resize_flag = 0;
static void                  set_term_dims(cmonkey* cm);
static void                  winch_handler(int sig);
static void                  signal_handler(int sig);
static void                  terminal_register_cleanup(void);
static void                  test_refill_words(cmonkey* cm);
static void                  cmonkey_handle_input(cmonkey* cm, cmonkey_input input);

static struct termios og_term;

#define FPS            60
#define NUM_RAND_WORDS 200
#define DEFAULT_TIME   60.f
#define WORDS_AHEAD    40
#define WORD_SPACING   2
#define LINE_SPACING   2



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
    genVec_destroy_stk(&cm->test.typed);
    genVec_destroy_stk(&cm->test.word_states);
}

void cmonkey_init_term(cmonkey* cm)
{
    struct sigaction sa = {.sa_handler = winch_handler};
    sigaction(SIGWINCH, &sa, NULL);
    terminal_register_cleanup();

    CHECK_WARN_RET(tcgetattr(STDIN_FILENO, &og_term) == -1, , "tcgetattr failed");

    struct termios raw = og_term;
    raw.c_lflag &= (tcflag_t) ~(ECHO | ICANON);
    CHECK_WARN_RET(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1, , "setting term attr failed");

    // must be after raw mode is applied
    input_init();

    tb_append_cstr(&cm->tb, "\033[?1049h"); // enter alternate screen
    tb_append_cstr(&cm->tb, CURSOR_HIDE);
    draw_clear(&cm->tb, &cm->t);
    tb_flush(&cm->tb);
}

void cmonkey_end_term(void)
{
    const char* cleanup = "\033[0m\033[?25h\033[?1049l";
    write(STDOUT_FILENO, cleanup, strlen(cleanup));
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &og_term);
}



void cmonkey_test_new(cmonkey* cm)
{
    // destroy old vecs if they exist
    genVec_destroy_stk(&cm->test.typed);
    genVec_destroy_stk(&cm->test.word_states);

    memset(&cm->test, 0, sizeof(cmonkey_test));

    genVec_init_stk(NUM_RAND_WORDS, sizeof(u32), NULL, &cm->test.typed);
    genVec_init_stk(NUM_RAND_WORDS, sizeof(WORD_STATE), NULL, &cm->test.word_states);

    cm->state = CMONKEY_WAITING;

    // pre-load the initial horizon so draw has words immediately
    test_refill_words(cm);
}

// Keep typed[] topped up so there are always WORDS_AHEAD words past curr_word.
static void test_refill_words(cmonkey* cm)
{
    cmonkey_test* test    = &cm->test;
    u32           total   = (u32)test->typed.size;
    u32           horizon = test->curr_word + WORDS_AHEAD;

    while (total < horizon) {
        if (queue_size(&cm->incoming) < 10) {
            wordbank_random_words_in_queue(&cm->wb, &cm->incoming);
        }

        u32 idx = DEQUEUE(&cm->incoming, u32);
        genVec_push(&test->typed, cast(idx));

        WORD_STATE st = WORD_PENDING;
        genVec_push(&test->word_states, cast(st));

        total++;
    }
}



static void handle_char(cmonkey* cm, char ch)
{
    cmonkey_test* test = &cm->test;

    if (ch == ' ') {
        // ignore leading space (nothing typed yet)
        if (test->curr_typed_len == 0) {
            return;
        }

        // look up the word the user was supposed to type
        u32         word_vec_idx = *(u32*)genVec_get_ptr(&test->typed, test->curr_word);
        Word*       w            = (Word*)genVec_get_ptr(cm->wb.words, word_vec_idx);
        const char* expected     = wordbank_word_at(&cm->wb, w->idx);

        // single strcmp — correct iff lengths match and every byte matches
        WORD_STATE state = (test->curr_typed_len == w->len && strncmp(test->curr_typed, expected, w->len) == 0)
                               ? WORD_CORRECT
                               : WORD_INCORRECT;

        *(WORD_STATE*)genVec_get_ptr(&test->word_states, test->curr_word) = state;

        if (state == WORD_CORRECT) {
            test->correct++;
        } else {
            test->incorrect++;
        }

        test->curr_word++;
        test->curr_typed_len = 0;
        test->curr_typed[0]  = '\0';

        test_refill_words(cm);
        return;
    }

    // normal printable: append up to buffer limit
    // allow typing past word length (it'll just show as all-wrong on commit)
    if (test->curr_typed_len < (u32)(sizeof(test->curr_typed) - 1)) {
        test->curr_typed[test->curr_typed_len++] = ch;
        test->curr_typed[test->curr_typed_len]   = '\0';
    }
}

static void handle_backspace(cmonkey* cm)
{
    cmonkey_test* test = &cm->test;
    if (test->curr_typed_len > 0) {
        test->curr_typed[--test->curr_typed_len] = '\0';
    }
}

static void handle_ctrl_backspace(cmonkey* cm)
{
    cmonkey_test* test   = &cm->test;
    test->curr_typed_len = 0;
    test->curr_typed[0]  = '\0';
}

static void cmonkey_handle_input(cmonkey* cm, cmonkey_input input)
{
    // Ctrl+C always quits regardless of state
    if (input.type == INPUT_CTRL_C) {
        cm->quit = true;
        return;
    }

    switch (cm->state) {

    case CMONKEY_WAITING:
        // ESC/Tab on waiting screen: nothing yet
        if (input.type == INPUT_ESCAPE) {
            cm->quit = true;
            return;
        }
        // any real keypress starts the test
        if (input.type == INPUT_CHAR) {
            cm->state = CMONKEY_UNDERGOING;
            // start timer here — elapsed_time counts up from 0
            cm->test.elapsed_time = 0.f;
            handle_char(cm, input.ch);
        }
        break;

    case CMONKEY_UNDERGOING:
        switch (input.type) {
        case INPUT_CHAR:
            handle_char(cm, input.ch);
            break;
        case INPUT_BACKSPACE:
            handle_backspace(cm);
            break;
        case INPUT_CTRL_BACKSPACE:
            handle_ctrl_backspace(cm);
            break;
        case INPUT_ESCAPE: /* TODO: pause / quit prompt */
            break;
        case INPUT_TAB:
            // restart
            wordbank_random_words_in_queue(&cm->wb, &cm->incoming);
            cmonkey_test_new(cm);
            break;
        default:
            break;
        }
        break;

    case CMONKEY_FINISHED:
        // any key on result screen restarts
        if (input.type == INPUT_TAB || input.type == INPUT_ESCAPE) {
            wordbank_random_words_in_queue(&cm->wb, &cm->incoming);
            cmonkey_test_new(cm);
        }
        break;
    }
}


void cmonkey_update(cmonkey* cm)
{
    // tick elapsed time while test is running
    if (cm->state == CMONKEY_UNDERGOING) {
        cm->test.elapsed_time += cm->timer.elapsed;
        if (cm->test.elapsed_time >= cm->test_time) {
            cm->state = CMONKEY_FINISHED;
        }
    }

    // drain all pending input this frame
    int n = input_poll(cm->inputs, 32);
    for (int i = 0; i < n; i++) {
        cmonkey_handle_input(cm, cm->inputs[i]);
    }

    // handle terminal resize
    if (resize_flag) {
        resize_flag = 0;
        set_term_dims(cm);
        // TODO: tb resize if new dims exceed original allocation
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
        draw_words_in_box_ex(&cm->tb, &cm->wb, textbox, &cm->test, &cm->t);
        break;
    }
    case CMONKEY_FINISHED:
        // TODO: result screen
        draw_text(&cm->tb, cm->rows / 2, (cm->cols / 2) - 10, &cm->t, "test complete");
        break;
    }

    Box timebox = { 6, 33, 3, 10 };
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
