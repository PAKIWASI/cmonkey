#include "cmonkey.h"
#include "Queue_single.h"

#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>


bool cmonkey_init(
    cmonkey* cm,
    const char* theme_path,
    const char* conf_path,
    const char* wordbank_path
) {
    memset(cm, 0, sizeof(*cm));

    // --- theme: try file, fall back to built-in ---
    cm->theme = theme_default();
    if (theme_path && !cmonkey_import_theme(&cm->theme, theme_path)) {
        WARN("theme '%s' not found, using default", theme_path);
    }

    // --- conf: try file, fall back to built-in ---
    cm->conf = conf_default();
    if (conf_path && !cmonkey_import_conf(&cm->conf, conf_path)) {
        WARN("conf '%s' not found, using defaults", conf_path);
    }

    // --- wordbank ---
    const char* wb_path = wordbank_path ? wordbank_path : DEFAULT_WORDBANK;
    cm->wordbank = wordbank_create(wb_path, NUM_LOAD_WORDS);
    if (!cm->wordbank) {
        WARN("failed to load wordbank '%s'", wb_path);
        return false;
    }

    cmonkey_set_term(cm);

    // --- tests ---
    cm->test.words = queue_create(NUM_LOAD_WORDS, sizeof(u32), NULL);
    cmonkey_new_test(cm);

    return true;
}

void cmonkey_end(cmonkey* cm)
{
    if (!cm) { return; }
    if (cm->wordbank) {
        wordbank_destroy(cm->wordbank);
        cm->wordbank = NULL;
    }
    if (cm->test.words) {
        queue_destroy(cm->test.words);
        cm->test.words = NULL;
    }
}

// --- terminal size ---
void cmonkey_set_term(cmonkey* cm)
{
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    cm->term.rows = ws.ws_row;
    cm->term.cols = ws.ws_col;
}

void cmonkey_new_test(cmonkey* cm)
{
    // words never run out, we just load more when
    // down to 20%  - should i do a check for that here?
    cm->test.curr_word = 0;
    cm->test.curr_char = 0;
    cm->test.correct   = 0;
    cm->test.state = TEST_UNDERGOING;
    // TODO: timer
}

void cmonkey_more_words(cmonkey* cm)
{
    wordbank_random_words_in_queue(cm->wordbank, cm->test.words);
}

