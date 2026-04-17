#ifndef GAME_H
#define GAME_H

#include "json_wordbank_loader.h"
#include <time.h>

// constants

// TODO: words should not run out
#define GAME_WORD_COUNT   30     // words per round
#define GAME_DURATION_S   15.0f  // seconds per round

// types

typedef enum {
    GS_WAITING,   // waiting for first keypress
    GS_TYPING,    // timer running
    GS_FINISHED,  // time up or all words typed
} GameState;

// Per-word typing result
typedef enum {
    WR_UNTOUCHED,
    WR_CORRECT,
    WR_WRONG,
} WordResult;

typedef struct {
    // word data
    WordBank* bank;
    genVec*   indices;        // u32 word-indices into bank (current round)
    u32       word_count;     // number of words this round

    // cursor
    u32       word_pos;       // which word we are on
    u32       char_pos;       // which char within that word

    // per-word result (WordResult, indexed by word_pos)
    WordResult results[GAME_WORD_COUNT];

    // timing
    GameState       state;
    struct timespec start_time;
    float           elapsed_s;

    // TODO: words/accuracy?
    // stats
    u32   correct_chars;
    u32   total_chars_typed;
} Game;


// api

// Initialise game, load wordbank, pick first round of words.
void game_init(Game* g, const char* wordbank_path);

// Tear down (does not free g itself).
void game_destroy(Game* g);

// Start a fresh round (new word selection, reset cursor/timer/stats).
void game_new_round(Game* g);

// Feed one character from the user. Returns true if the round just ended.
bool game_input(Game* g, int ch);

// Update elapsed time. Call every loop iteration.
// Returns true if time just ran out.
bool game_tick(Game* g);

// Helpers for the UI layer
const char* game_current_word(const Game* g);
float       game_wpm(const Game* g);
float       game_accuracy(const Game* g);
float       game_time_left(const Game* g);

#endif // GAME_H
