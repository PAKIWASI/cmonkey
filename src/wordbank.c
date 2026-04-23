#include "wordbank.h"
#include "arena_single.h"
#include "random_single.h"
#include "wc_macros_single.h"

#define JSMN_PARENT_LINKS
#include "jsmn.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LOAD_WORDS 100000 // 100K


// file helpers

// reads entire json file into a buffer (null terminated)
static char* read_file(const char* filename, u32* out_size)
{
    FILE* f = fopen(filename, "rb");
    if (!f) {
        WARN("cannot open '%s'", filename);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz <= 0) {
        WARN("file '%s' empty", filename);
        fclose(f);
        return NULL;
    }

    char* buf = malloc((size_t)sz + 1);
    if (!buf) {
        WARN("OOM reading '%s'", filename);
        fclose(f);
        return NULL;
    }

    size_t n = fread(buf, 1, (size_t)sz, f);
    fclose(f);

    if ((long)n != sz) {
        WARN("short read on '%s'", filename);
        free(buf);
        return NULL;
    }

    buf[sz]   = '\0';
    *out_size = (u32)sz;
    return buf;
}

// parse json (2 phases) and build token arr
static jsmntok_t* jsmn_parse_json(char* json_buf, u32 json_len, jsmn_parser* parser, int* num_tokens)
{
    // Two-pass jsmn parse
    // Pass 1: tokens=NULL → jsmn counts tokens, returns how many
    jsmn_parser p;
    jsmn_init(&p);

    int n_tokens = jsmn_parse(&p, json_buf, json_len, NULL, 0);
    CHECK_WARN_RET(n_tokens <= 0, NULL, "jsmn count pass failed: %d", n_tokens);

    // Pass 2: allocate token array, parse for real
    jsmntok_t* toks = malloc((size_t)n_tokens * sizeof(jsmntok_t));
    CHECK_WARN_RET(!toks, NULL, "OOM tokens");

    // have to re-init
    jsmn_init(&p);
    // build tokens
    int r = jsmn_parse(&p, json_buf, json_len, toks, (u32)n_tokens);
    if (r < 0) {
        free(toks);
        WARN("jsmn second parse failed");
        return NULL;
    }

    // this has to be true, otherwise, invalid json
    if (toks[0].type != JSMN_OBJECT) {
        free(toks);
        WARN("JSON root is not an object");
        return NULL;
    }

    *parser     = p;
    *num_tokens = n_tokens;
    return toks;
}


/*
Find "words" array token

    Token layout (JSMN_PARENT_LINKS, example {"words":["a","b"]}):
    [0] OBJECT  size=1  parent=-1
    [1] STRING "words"  size=1  parent=0   <- key
    [2] ARRAY           size=2  parent=1   <- value  (this is words_array_idx)
    [3] STRING "a"      size=0  parent=2
    [4] STRING "b"      size=0  parent=2

    Every JSON key is a STRING whose parent is the enclosing OBJECT index.
    Its value token is at key_index + 1.
*/
static int find_words_arr(char* json_buf, jsmntok_t* toks, int num_tokens)
{
    int words_arr = -1;
    for (int i = 1; i < num_tokens; i++) {
        jsmntok_t* t = &toks[i];
        if (t->type == JSMN_STRING && t->parent == 0) {
            int len = t->end - t->start;
            if (len == 5 && strncmp(json_buf + t->start, "words", 5) == 0) {
                words_arr = i + 1;
                break;
            }
        }
    }

    CHECK_WARN_RET(words_arr < 0 || toks[words_arr].type != JSMN_ARRAY, -1, "no 'words' array in JSON");

    return words_arr;
}

WordBank* wordbank_create(const char* filename, u32 num_random_words)
{
    // Read file into a temporary heap buffer
    u32   json_len = 0;
    char* json_buf = read_file(filename, &json_len);
    CHECK_WARN_RET(!json_buf, NULL, "failed to read '%s'", filename);

    int         num_tokens = 0;
    jsmn_parser parser;
    // get token array, tokens stored inline
    jsmntok_t* toks = jsmn_parse_json(json_buf, json_len, &parser, &num_tokens);
    CHECK_WARN_RET(!toks, NULL, "jsmn parse failed");

    WordBank* wb = NULL;

    // returns the offset to value of "words" key: the words array itself
    int words_arr = find_words_arr(json_buf, toks, num_tokens);
    if (words_arr == -1) {
        WARN("words array invalid");
        goto cleanup;
    }

    u32 num_words = (u32)toks[words_arr].size;
    if (num_words == 0) {
        WARN("'words' array is empty");
        goto cleanup;
    }

    // Seed RNG once. If we have >= 2*MAX_LOAD_WORDS words, pick a random
    // contiguous block of MAX_LOAD_WORDS starting at a random word index.
    // For smaller banks, load everything as-is.

    pcg32_rand_seed_time();

    u32 load_count   = num_words;
    u32 start_offset = 0;
    if (num_words >= MAX_LOAD_WORDS * 2) {
        start_offset = pcg32_rand_bounded(num_words);
        load_count   = MAX_LOAD_WORDS;
    }

    wb = malloc(sizeof(WordBank));
    if (!wb) {
        WARN("OOM WordBank");
        goto cleanup;
    }

    // Size pass: walk tokens starting at start_offset-th matching word, wrapping around
    // calculate how much bytes are needed
    u64 bytes_needed = 0;
    u32 skipped      = 0;
    u32 found        = 0;
    int i            = words_arr + 1;
    while (found < load_count) {
        if (i >= num_tokens) {
            i = words_arr + 1; // wrap
        }
        if (toks[i].parent == words_arr && toks[i].type == JSMN_STRING) {
            if (skipped < start_offset) {
                skipped++;
            } else {
                bytes_needed += (u64)(toks[i].end - toks[i].start) + 1;
                found++;
            }
        }
        i++;
    }

    wb->arena = arena_create(bytes_needed);
    if (!wb->arena) {
        WARN("arena_create failed");
        goto cleanup;
    }
    
    // size is equal to num of loaded words
    wb->words = genVec_init(load_count, sizeof(u32), NULL);

    // Copy pass: identical walk
    skipped = 0;
    found   = 0;
    i       = words_arr + 1;
    while (found < load_count) {
        if (i >= num_tokens) {
            i = words_arr + 1; // wrap
        }
        jsmntok_t* t = &toks[i];
        if (t->parent == words_arr && t->type == JSMN_STRING) {
            if (skipped < start_offset) {
                skipped++;
            } else {
                int wlen   = t->end - t->start;
                u32 offset = (u32)arena_used(wb->arena);

                char* dest = (char*)arena_alloc(wb->arena, (u64)wlen + 1);
                CHECK_FATAL(!dest, "arena exhausted");

                memcpy(dest, json_buf + t->start, (size_t)wlen);
                dest[wlen] = '\0';

                genVec_push(wb->words, (u8*)&offset);
                found++;
            }
        }
        i++;
    }

    // Pre-allocate scratch array for wordbank_random_words (avoids per-call malloc)
    wb->scratch = malloc(load_count * sizeof(u32));
    if (!wb->scratch) {
        WARN("OOM scratch");
        goto cleanup;
    }
    for (u32 k = 0; k < load_count; k++) {
        wb->scratch[k] = k;
    }

                            // DEBUG: NUM_RAND_WORDS
    if (num_random_words == 0) {
        WARN("num_random_words can't be 0");
        goto cleanup;
    }
    wb->num_random_words = num_random_words;
    wb->swapped_j = malloc(num_random_words * sizeof(u32));
    if (!wb->swapped_j) {
        WARN("OOM scratch");
        goto cleanup;
    }


    free(toks);
    free(json_buf);

    LOG("loaded %u words (%lu bytes)", found, arena_used(wb->arena));
    return wb;

cleanup:
    free(json_buf);
    free(toks);
    if (wb) {
        if (wb->arena) { arena_release(wb->arena); }
        if (wb->words) { genVec_destroy(wb->words); }
        if (wb->scratch) { free(wb->scratch); }
        free(wb);
    }
    return NULL;
}

void wordbank_destroy(WordBank* wb)
{
    if (!wb) {
        return;
    }

    LOG("arena used: %lu", arena_used(wb->arena));

    arena_release(wb->arena);
    genVec_destroy(wb->words);
    free(wb->scratch);
    free(wb->swapped_j);
    free(wb);
}


void wordbank_random_words(WordBank* wb, u32* buff, u32 buff_size)
{
    CHECK_WARN_RET(!wb, , "null wordbank");
    CHECK_WARN_RET(wb->num_random_words != buff_size, , "mismatch");

    u32 total = (u32)wb->words->size;

    for (u32 i = 0; i < buff_size; i++) {
        u32 j = i + pcg32_rand_bounded(total - i);
        wb->swapped_j[i] = j;

        u32 tmp         = wb->scratch[i];
        wb->scratch[i]  = wb->scratch[j];
        wb->scratch[j]  = tmp;

        buff[i] = wb->scratch[i];
    }

    // Undo swaps in reverse — restores scratch to identity without full reset
    for (u32 i = buff_size; i-- > 0;) {
        u32 j          = wb->swapped_j[i];
        u32 tmp        = wb->scratch[i];
        wb->scratch[i] = wb->scratch[j];
        wb->scratch[j] = tmp;
    }
}

void wordbank_random_words_in_queue(WordBank* wb, Queue* q)
{
    CHECK_WARN_RET(!wb, , "wordbank is null");
    CHECK_WARN_RET(!q, , "queue is null");

    u32 total = (u32)wb->words->size;
    u32 num   = wb->num_random_words;

    for (u32 i = 0; i < num; i++) {
        u32 j = i + pcg32_rand_bounded(total - i);
        wb->swapped_j[i] = j;

        u32 tmp         = wb->scratch[i];
        wb->scratch[i]  = wb->scratch[j];
        wb->scratch[j]  = tmp;

        enqueue(q, cast(wb->scratch[i]));
        // ENQUEUE(q, wb->scratch[i]);
    }

    // Undo swaps in reverse — restores scratch to identity without full reset
    for (u32 i = num; i --> 0;) {
        u32 j          = wb->swapped_j[i];
        u32 tmp        = wb->scratch[i];
        wb->scratch[i] = wb->scratch[j];
        wb->scratch[j] = tmp;
    }
}


