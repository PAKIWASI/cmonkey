

#define FILE_PATH_ENG "/home/wasi/Documents/projects/c/cmonkey/data/english.json"
#define FILE_PATH_ENG1K "/home/wasi/Documents/projects/c/cmonkey/data/english_1k.json"
#define FILE_PATH_ENG5K "/home/wasi/Documents/projects/c/cmonkey/data/english_10k.json"
#define FILE_PATH_ENG10K "/home/wasi/Documents/projects/c/cmonkey/data/english_5k.json"
#define FILE_PATH_ENG25K "/home/wasi/Documents/projects/c/cmonkey/data/english_25k.json"
#define FILE_PATH_ENG450K "/home/wasi/Documents/projects/c/cmonkey/data/english_450k.json"
#define FILE_PATH_ENG_MISSPELLED "/home/wasi/Documents/projects/c/cmonkey/data/english_commonly_misspelled.json"
#define FILE_PATH_ARABIC "/home/wasi/Documents/projects/c/cmonkey/data/arabic.json"
#define FILE_PATH_C "/home/wasi/Documents/projects/c/cmonkey/data/code_c.json"

#define CURR_FILE FILE_PATH_ENG450K


#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

static struct termios orig_termios;

void term_raw_mode(void) 
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);  // no echo, char-by-char input
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void term_restore(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void term_get_size(int *rows, int *cols) {
    struct winsize ws;
    // terminal io control get win size
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    *rows = ws.ws_row;
    *cols = ws.ws_col;
}


int main(void)
{
    term_raw_mode();

    return 0;
}


