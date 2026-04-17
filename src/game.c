#include "game.h"
#include "json_wordbank_loader.h"
#include "random_single.h"
#include <string.h>
#include <time.h>


// internal helpers

#define time_diff_s(a, b) \
    (float)((b)->tv_sec  - (a)->tv_sec)\
    + ((float)((b)->tv_nsec - (a)->tv_nsec) / 1e9f);\


// api

void game_init(Game* g, const char* wordbank_path)
{
    memset(g, 0, sizeof(*g));
    pcg32_rand_seed_time();
    g->bank    = wordbank_create(wordbank_path);
    g->indices = NULL;
    CHECK_FATAL(!g->bank, "failed to load wordbank from '%s'", wordbank_path);
    game_new_round(g);
}

void game_destroy(Game* g)
{
    if (g->indices) { genVec_destroy(g->indices); g->indices = NULL; }
    if (g->bank)    { wordbank_destroy(g->bank);  g->bank    = NULL; }
}

void game_new_round(Game* g)
{
    if (g->indices) {
        genVec_destroy(g->indices);
    }
    g->indices    = wordbank_random_words(g->bank, GAME_WORD_COUNT);
    g->word_count = (u32)g->indices->size;

    g->word_pos          = 0;
    g->char_pos          = 0;
    g->correct_chars     = 0;
    g->total_chars_typed = 0;
    g->elapsed_s         = 0.0f;
    g->state             = GS_WAITING;

    memset(g->results, WR_UNTOUCHED, sizeof(g->results));
}

// Returns true if the round just finished.
bool game_input(Game* g, int ch)
{
    if (g->state == GS_FINISHED) { return false; }

    // Start timer on first real keypress
    if (g->state == GS_WAITING) {
        timespec_get(&g->start_time, TIME_UTC);
        g->state = GS_TYPING;
    }

    if (g->word_pos >= g->word_count) {
        g->state = GS_FINISHED;
        return true;
    }

    const char* word    = game_current_word(g);
    u32         wlen    = (u32)strlen(word);

    g->total_chars_typed++;

    if (ch == ' ') {
        // Space advances to next word (counts current word as done)
        if (g->char_pos == wlen) {
            g->results[g->word_pos] = WR_CORRECT;
            g->correct_chars += wlen;
        } else {
            g->results[g->word_pos] = WR_WRONG;
        }
        g->word_pos++;
        g->char_pos = 0;

        if (g->word_pos >= g->word_count) {
            g->state = GS_FINISHED;
            return true;
        }
        return false;
    }

    // Regular character — compare against expected
    if (g->char_pos < wlen) {
        if ((char)ch == word[g->char_pos]) {
            g->char_pos++;
        } else {

        }
        // wrong char: don't advance (monkeytype-style: must retype correctly)
        // if you want lenient mode, just always increment char_pos here
    }

    return false;
}


// Returns true if time just ran out.
bool game_tick(Game* g)
{
    if (g->state != GS_TYPING) { return false; }

    struct timespec now;
    timespec_get(&now, TIME_UTC);
    g->elapsed_s = time_diff_s(&g->start_time, &now);

    if (g->elapsed_s >= GAME_DURATION_S) {
        g->elapsed_s = GAME_DURATION_S;
        g->state     = GS_FINISHED;
        return true;
    }
    return false;
}

const char* game_current_word(const Game* g)
{
    if (!g->indices || g->word_pos >= g->word_count) { return ""; }
    u32 idx = *(u32*)genVec_get_ptr(g->indices, g->word_pos);
    return wordbank_word_at(g->bank, idx);
}

float game_wpm(const Game* g)
{
    float duration = g->elapsed_s > 0.0f ? g->elapsed_s : 1.0f;
    // WPM = (correct chars / 5) / (elapsed minutes)
    return ((float)g->correct_chars / 5.0f) / (duration / 60.0f);
}

float game_accuracy(const Game* g)
{
    if (g->total_chars_typed == 0) { return 100.0f; }
    return 100.0f * (float)g->correct_chars / (float)g->total_chars_typed;
}

float game_time_left(const Game* g)
{
    float left = GAME_DURATION_S - g->elapsed_s;
    return left < 0.0f ? 0.0f : left;
}

