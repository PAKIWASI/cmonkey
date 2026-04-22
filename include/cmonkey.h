#ifndef CMONKEY_H
#define CMONKEY_H

#include "config.h"
#include "cursor.h"
#include "wordbank.h"


#define DEFAULT_TIME_SEC 15.0f
#define NUM_LOAD_WORDS  60      // how many words to load each time
// if 20% left, load NUM_LOAD_WORDS more

#define DEFAULT_THEME_PATH  "themes/dracula.theme"
#define DEFAULT_CONF_PATH   "themes/cmonkey.conf"
#define DEFAULT_WORDBANK    "wordbanks/english.json"


// TODO: we shouldn't run out of words !

typedef enum {
    TEST_WAITING,
    TEST_UNDERGOING,
    TEST_FINISHED,
} TEST_STATE;


typedef struct {
    genVec* words;      // u32 indexes into word array we got
    // TODO: a queue makes sense here, we can load more when we want to back of queue
    u32   curr_word;   // which word the user is on
    u8    curr_char;   // which char within curr_word
    u32   correct;     // correctly completed words
    float time_total;  // test duration in seconds
    float time_left;   // counts down to 0
    TEST_STATE state;
    // test timer
} cmonkey_test;


typedef struct {
    u32 rows, cols;    // current terminal dimensions
} cmonkey_term;


typedef struct {
    cmonkey_theme   theme;
    cmonkey_conf    conf;
    WordBank*       wordbank;
    cmonkey_test    test;
    cmonkey_term    term;
    cmonkey_cursor  cursor;
    // fps timer
    bool            quit;
} cmonkey;


// Initialise app state: loads config/theme, creates wordbank, seeds test.
// Returns false and prints reason if anything critical fails.
bool cmonkey_init(cmonkey* cm,
                  const char* theme_path,
                  const char* conf_path,
                  const char* wordbank_path);

// Tear down — safe to call even if init failed partway.
void cmonkey_free(cmonkey* cm);

// Start a fresh test (pick new words, reset counters).
void cmonkey_new_test(cmonkey* cm);

// current test needs more words
void cmonkey_more_words(cmonkey* cm);

// TODO: render here?


#endif // CMONKEY_H
