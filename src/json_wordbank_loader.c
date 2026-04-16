#include "json_wordbank_loader.h"
#include "arena_single.h"

#define JSMN_PARENT_LINKS
#include "jsmn.h"

#include "gen_vector_single.h"
#include "random_single.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// TODO: do we even need '\0' at the end of each word because we
// have indexes stored in vec to the start of each word
// so we could easily calculate the length of the word


// file helpers

// reads entire json file into a buffer (null terminated)
static char* read_file(const char* filename, u32* out_size)
{
    FILE* f = fopen(filename, "rb");
    if (!f) { WARN("cannot open '%s'", filename); return NULL; }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz <= 0) { WARN("file '%s' empty", filename); fclose(f); return NULL; }

    char* buf = malloc((size_t)sz + 1);
    if (!buf)  { WARN("OOM reading '%s'", filename); fclose(f); return NULL; }

    size_t n = fread(buf, 1, (size_t)sz, f);
    fclose(f);

    if ((long)n != sz) { WARN("short read on '%s'", filename); free(buf); return NULL; }

    buf[sz]   = '\0';
    *out_size = (u32)sz;
    return buf;
}

static jsmntok_t* jsmn_parse_json(char* json_buf, u32 json_len, jsmn_parser* parser, int* num_tokens)
{
    // Two-pass jsmn parse
    // Pass 1: tokens=NULL → jsmn counts tokens, returns how many
    jsmn_parser p;
    jsmn_init(&p);

    int n_tokens = jsmn_parse(&p, json_buf, json_len, NULL, 0);
    CHECK_FATAL(n_tokens <= 0, "jsmn count pass failed: %d", n_tokens);

    // BUG:
    printf("\nnum_tokens: %d\n", n_tokens);

    // Pass 2: allocate token array, parse for real
    jsmntok_t* toks = malloc((size_t)n_tokens * sizeof(jsmntok_t));
    CHECK_FATAL(!toks, "OOM tokens");

    // have to re-init
    jsmn_init(&p);
    int r = jsmn_parse(&p, json_buf, json_len, toks, (u32)n_tokens);
    CHECK_FATAL(r < 0, "jsmn fill pass failed: %d", r);

    // this has to be true, otherwise, invalid json
    CHECK_FATAL(toks[0].type != JSMN_OBJECT, "JSON root is not an object");

    *parser = p;
    *num_tokens = n_tokens;
    return toks;
}


WordBank* wordbank_create(const char* filename)
{
    // Read file into a temporary heap buffer
    u32   json_len = 0;
    char* json_buf = read_file(filename, &json_len);
    CHECK_WARN_RET(!json_buf, NULL, "failed to read '%s'", filename);


    int num_tokens;
    jsmn_parser parser;
    jsmntok_t* toks = jsmn_parse_json(json_buf, json_len, &parser, &num_tokens);


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

    if (words_arr < 0 || toks[words_arr].type != JSMN_ARRAY) {
        WARN("no 'words' array in JSON");
        free(toks); free(json_buf);
        return NULL;
    }

    u32 num_words = (u32)toks[words_arr].size;
    if (num_words == 0) {
        WARN("'words' array is empty");
        free(toks); free(json_buf);
        return NULL;
    }

    // BUG: wrong
    printf("\nnum_words: %d\n", num_words);

    // Calculate exact arena size: sum(word_len) + num_words NUL bytes
    u64 bytes_needed = 0;
    u32 found = 0;
    for (int i = words_arr + 1; i < num_tokens && found < num_words; i++) {
        if (toks[i].parent == words_arr && toks[i].type == JSMN_STRING) {
            bytes_needed += (u64)(toks[i].end - toks[i].start) + 1;
            found++;
        }
    }

    // Allocate WordBank and arena
    WordBank* wb = malloc(sizeof(WordBank));
    if (!wb) {
        WARN("OOM WordBank");
        free(toks);
        free(json_buf);
        return NULL;
    }

    Arena* a = arena_create(bytes_needed);
    if (!a) { 
        WARN("arena_create failed");
        free(wb);
        free(toks);
        free(json_buf);
        return NULL;
    }
    wb->arena = *a;
    free(a); // we copied the struct; the base pointer is still valid

    wb->words = *genVec_init(num_words, sizeof(u32), NULL);

    // Copy words into arena, store byte offset per word
    found = 0;
    for (int i = words_arr + 1; i < num_tokens && found < num_words; i++) {
        jsmntok_t* t = &toks[i];
        if (t->parent != words_arr || t->type != JSMN_STRING) { continue; }

        int  wlen   = t->end - t->start;
        u32  offset = (u32)arena_used(&wb->arena);

        // this aligns up by default but we added ARENA_DEFAULT_ALIGNMENT 0 to wc_imp
        char* dest = (char*)arena_alloc(&wb->arena, (u64)wlen + 1);
        CHECK_FATAL(!dest, "arena exhausted — bytes_needed calculation bug");

        memcpy(dest, json_buf + t->start, (size_t)wlen);
        dest[wlen] = '\0';

        // BUG: does print all words correctly but arena used is wrong
        printf("%d: %s: %lu\n", i, dest, arena_used(&wb->arena));
        
        genVec_push(&wb->words, (u8*)&offset);
        found++;
    }

    // BUG: indexes wrong
    genVec_print(&wb->words, wc_print_u32);
    putchar('\n');

    // Free temporaries — all string data is now in the arena
    free(toks);
    free(json_buf);

    // BUG: shows 5 bytes
    LOG("wordbank: loaded %u words (%lu bytes)", found, arena_used(&wb->arena));
    return wb;
}


void wordbank_destroy(WordBank* wb)
{
    if (!wb) { return; }
    free(wb->arena.base);
    genVec_destroy_stk(&wb->words);
    free(wb);
}


const char* wordbank_word_at(WordBank* wb, u32 i)
{
    u32 offset = *(u32*)genVec_get_ptr(&wb->words, i);
    return (const char*)(wb->arena.base + offset);
}


const char* wordbank_random_word(WordBank* wb)
{
    CHECK_WARN_RET(!wb || wb->words.size == 0, NULL, "empty wordbank");
    u32 idx = pcg32_rand_bounded((u32)wb->words.size);
    return wordbank_word_at(wb, idx);
}


// Returns a genVec* of u32 word-indices (not copies).
// Partial Fisher-Yates: O(N) time, no allocations beyond the scratch array.
genVec* wordbank_random_words(WordBank* wb, u32 count)
{
    CHECK_WARN_RET(!wb, NULL, "null wordbank");

    u32 total = (u32)wb->words.size;
    if (count > total) {
        WARN("requested %u words, bank has %u — clamping", count, total);
        count = total;
    }

    u32* scratch = malloc(total * sizeof(u32));
    CHECK_FATAL(!scratch, "OOM in wordbank_random_words");
    for (u32 i = 0; i < total; i++) { scratch[i] = i; }

    for (u32 i = 0; i < count; i++) {
        u32 j       = i + pcg32_rand_bounded(total - i);
        u32 tmp     = scratch[i];
        scratch[i]  = scratch[j];
        scratch[j]  = tmp;
    }

    genVec* result = genVec_init(count, sizeof(u32), NULL);
    for (u32 i = 0; i < count; i++) {
        genVec_push(result, (u8*)&scratch[i]);
    }

    free(scratch);
    return result;
}

