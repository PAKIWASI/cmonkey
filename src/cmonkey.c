#include "cmonkey.h"
#include "Queue_single.h"
#include "config.h"
#include "wordbank.h"
#include <asm-generic/ioctls.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <unistd.h>


#define NUM_RAND_WORDS 200

static void set_term_dims(cmonkey* cm);
static volatile sig_atomic_t resize_flag = 0;
static void winch_catcher(void);
static void winch_handler(int sig); // TODO: can i make this static?



void cmonkey_begin(cmonkey* cm, const char* wb_path, const char* theme_path, const char* conf_path)
{
    cm->wb = wordbank_create(wb_path, NUM_RAND_WORDS);
    CHECK_FATAL(!cm->wb, "wordbank creation failed");

    cm->q = queue_create((u64)NUM_RAND_WORDS * 2, sizeof(u32), NULL);

    cm->t = theme_load(theme_path);

    set_term_dims(cm);


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

// When the user resizes the terminal, the kernel sends a SIGWINCH signal
// You can catch it and re‑fetch the dimensions
void winch_handler(int sig)
{
    (void)sig;            // unused parameter
    resize_flag = 1;      // set flag for main loop
}

// TODO: do this once on top?
void winch_catcher(void)
{
    struct sigaction sa = { .sa_handler = winch_handler };
    sigaction(SIGWINCH, &sa, NULL);

    /*
        while (1) {
        pause();  // wait until any signal arrives
        if (resize_flag) {  // TODO: can do this in main loop
            resize_flag = 0;
            set_term_dims(&cm);
        }
    */
}


