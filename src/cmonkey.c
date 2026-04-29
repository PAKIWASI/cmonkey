#include "cmonkey.h"
#include "Queue_single.h"
#include "common_single.h"

#include <termios.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <asm-generic/ioctls.h>


#define NUM_RAND_WORDS 200

static volatile sig_atomic_t resize_flag = 0;
static void set_term_dims(cmonkey* cm);
void winch_handler(int sig);    // TODO: can i make this static?

// Store original terminal settings for restoration
static struct termios og_term;


void cmonkey_create(cmonkey* cm, const char* wb_path, const char* theme_path, const char* conf_path)
{
    cm->wb = wordbank_create(wb_path, NUM_RAND_WORDS);
    CHECK_FATAL(!cm->wb, "wordbank creation failed");

    cm->q = queue_create((u64)NUM_RAND_WORDS * 2, sizeof(u32), NULL);

    cm->t = theme_load(theme_path);
    CHECK_FATAL(!cm->t, "theme load failed");

    cm->c = config_load(conf_path);
    CHECK_FATAL(!cm->c, "config load failed");

    set_term_dims(cm);

    cm->quit = false;
}

void cmonkey_destroy(cmonkey* cm)
{
    wordbank_destroy(cm->wb);

    queue_destroy(cm->q);

    theme_unload(cm->t);

    config_unload(cm->c);
}

void cmonkey_begin(cmonkey* cm)
{
    // set SIGWINCH handler
    struct sigaction sa = { .sa_handler = winch_handler };
    sigaction(SIGWINCH, &sa, NULL);

    CHECK_WARN_RET(tcgetattr(STDIN_FILENO, &og_term) == -1,,
                   "tcgetattr failed");

    // Set up terminal for raw mode
    struct termios raw = og_term;

    // Input modes: disable break, CR-to-NL, parity check, strip high bit,
    // enable 8-bit input, disable XON/XOFF flow control
    // raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    
    // Output modes: disable post-processing (e.g., \n -> \r\n)
    // raw.c_oflag &= ~(OPOST);
    
    // Control modes: set 8-bit characters
    // raw.c_cflag |= (CS8);
    
    // Local modes: disable echo, canonical mode, extended processing,
    // signal characters (like ^C)
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    
    // Special characters: minimum characters for read, timeout in 0.1s
    raw.c_cc[VMIN] = 0;   // Return immediately with what's available
    raw.c_cc[VTIME] = 1;  // Wait up to 0.1 seconds
    
    // Apply changes after draining output
    CHECK_WARN_RET(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1,,
                   "setting term attr failed");
    
}

// terminal restore:
//     tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);

void cmonkey_update(cmonkey* cm)
{

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


