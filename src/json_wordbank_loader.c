#include "json_wordbank_loader.h"

#define JSMN_PARENT_LINKS
#include "jsmn.h"

#include "gen_vector_single.h"
#include "random_single.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



// HOW JSMN WORKS
//
// jsmn is a zero-copy, non-allocating tokeniser. It does NOT build a tree or
// copy any strings. Instead it gives you a flat array of jsmntok_t, each of
// which is just a (type, start, end, size) descriptor into the original JSON
// string you keep in memory.
//
// The JSON string must stay alive for as long as you use the tokens.
//
// Token fields:
//   .type   — JSMN_OBJECT | JSMN_ARRAY | JSMN_STRING | JSMN_PRIMITIVE
//   .start  — byte offset of first character (inclusive) in the JSON string
//   .end    — byte offset past the last character (exclusive)
//   .size   — number of direct children (for objects/arrays)
//             For a string VALUE this is 0.
//             For a string KEY   this is 1 (its value counts as child).
//
// With JSMN_PARENT_LINKS enabled each token also has:
//   .parent — index of the parent token in the token array (-1 = root)
//
// Two-pass usage pattern (what we do below):
//   Pass 1 — call jsmn_parse with tokens=NULL to count how many tokens the
//             JSON contains.  jsmn returns that count (or a negative error).
//   Pass 2 — allocate exactly that many jsmntok_t and call jsmn_parse again.
//
// Extracting a string token:
//   const char *word = json + tok.start;
//   int         len  = tok.end - tok.start;
//   // word[0..len-1] is the raw UTF-8 text, NOT NUL-terminated.


// internal helpers

// Read the entire file into a heap buffer. Returns NULL on error.
// Caller is responsible for free()-ing the returned pointer.
// Sets *out_size to the number of bytes read (not including the '\0' we append).
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
        WARN("file '%s' is empty or unreadable", filename);
        fclose(f);
        return NULL;
    }

    // +1 so we can append '\0' — jsmn only needs the length so this is just
    // for safety if any C string function is ever called on the buffer.
    char* buf = malloc((size_t)sz + 1);
    if (!buf) {
        WARN("OOM reading '%s'", filename);
        fclose(f);
        return NULL;
    }

    size_t read = fread(buf, 1, (size_t)sz, f);
    fclose(f);

    if ((long)read != sz) {
        WARN("short read on '%s'", filename);
        free(buf);
        return NULL;
    }

    buf[sz]   = '\0';
    *out_size = (u32)sz;
    return buf;
}

// Expected JSON shape:
//   { "words": ["apple", "banana", "cherry", ...] }
//
// Strategy:
//   1. Read whole file into a temp heap buffer (json_buf).
//   2. Two-pass jsmn parse to get the token array.
//   3. Walk tokens to locate the "words" key, then iterate its array children.
//   4. First pass over word tokens: accumulate total byte count needed for the
//      arena (sum of word lengths + one '\0' per word).
//   5. Allocate arena and WordBank.
//   6. Second pass over word tokens: copy each word + '\0' into the arena,
//      push its arena offset into wb->words.
//   7. Free the json_buf — all data now lives in the arena.
WordBank* wordbank_create(const char* filename)
{
    // Read file
    u32   json_len = 0;
    char* json_buf = read_file(filename, &json_len);
    CHECK_WARN_RET(!json_buf, NULL, "failed to read '%s'", filename);

    // Count tokens (pass 1)
    jsmn_parser parser;
    jsmn_init(&parser);

    // Pass NULL for tokens — jsmn just counts and returns the number needed.
    int num_tokens = jsmn_parse(&parser, json_buf, json_len, NULL, 0);
    if (num_tokens <= 0) {
        WARN("jsmn_parse (count pass) failed: %d", num_tokens);
        free(json_buf);
        return NULL;
    }

    // Parse into real token array (pass 2)
    jsmntok_t* tokens = malloc((size_t)num_tokens * sizeof(jsmntok_t));
    if (!tokens) {
        WARN("OOM allocating %d tokens", num_tokens);
        free(json_buf);
        return NULL;
    }

    jsmn_init(&parser); // must reinit before second parse
    int r = jsmn_parse(&parser, json_buf, json_len, tokens, (u32)num_tokens);
    if (r < 0) {
        WARN("jsmn_parse (fill pass) failed: %d", r);
        free(tokens);
        free(json_buf);
        return NULL;
    }

    // Sanity: root must be an object.
    if (tokens[0].type != JSMN_OBJECT) {
        WARN("JSON root is not an object");
        free(tokens);
        free(json_buf);
        return NULL;
    }

    // Find the "words" array
    //
    // Token layout for {"words": ["a","b"]} with PARENT_LINKS:
    //   [0]  OBJECT  start=0  end=…  size=1   parent=-1
    //   [1]  STRING  "words"         size=1   parent=0    ← key
    //   [2]  ARRAY                   size=N   parent=1    ← value of "words"
    //   [3]  STRING  "a"             size=0   parent=2    ← element 0
    //   [4]  STRING  "b"             size=0   parent=2    ← element 1
    //   …
    //
    // We walk the top-level object children (every key is a STRING with
    // parent == 0 that has size==1 meaning it owns a value token).

    int words_array_idx = -1; // index in tokens[] of the JSMN_ARRAY

    for (int i = 1; i < num_tokens; i++) {
        jsmntok_t* t = &tokens[i];

        // A top-level key: string whose parent is the root object (index 0).
        if (t->type == JSMN_STRING && t->parent == 0) {
            int key_len = t->end - t->start;
            if (key_len == 5 && strncmp(json_buf + t->start, "words", 5) == 0) {
                // The value token immediately follows the key token.
                words_array_idx = i + 1;
                break;
            }
        }
    }

    if (words_array_idx < 0 || tokens[words_array_idx].type != JSMN_ARRAY) {
        WARN("JSON has no 'words' array");
        free(tokens);
        free(json_buf);
        return NULL;
    }

    u32 num_words = (u32)tokens[words_array_idx].size;
    if (num_words == 0) {
        WARN("'words' array is empty");
        free(tokens);
        free(json_buf);
        return NULL;
    }

    // Calculate arena size
    //
    // Each word token whose parent == words_array_idx is a word string.
    // We need: sum of (tok.end - tok.start) bytes  +  num_words '\0' bytes.
    //
    // We also need padding headroom; arena_alloc may align internally, but
    // since we're storing plain chars we add a small safety margin.

    u64 bytes_needed = 0;
    u32 words_found  = 0;

    for (int i = words_array_idx + 1; i < num_tokens && words_found < num_words; i++) {
        jsmntok_t* t = &tokens[i];
        if (t->parent == words_array_idx && t->type == JSMN_STRING) {
            bytes_needed += (u64)(t->end - t->start) + 1; // +1 for '\0'
            words_found++;
        }
    }

    // Allocate WordBank + arena

    WordBank* wb = malloc(sizeof(WordBank));
    if (!wb) {
        WARN("OOM allocating WordBank");
        free(tokens);
        free(json_buf);
        return NULL;
    }

    wb->arena = *arena_create(bytes_needed);
    // wb->words stores u32 arena offsets — one per word — POD so ops=NULL.
    wb->words = *genVec_init(num_words, sizeof(u32), NULL);

    //  Copy words into arena, record offsets

    words_found = 0;
    for (int i = words_array_idx + 1; i < num_tokens && words_found < num_words; i++) 
    {
        jsmntok_t* t = &tokens[i];
        if (t->parent != words_array_idx || t->type != JSMN_STRING) { continue; }

        int  word_len = t->end - t->start;
        u64  offset   = arena_used(&wb->arena); // byte offset BEFORE alloc

        // Allocate space for word + NUL terminator in the arena.
        char* dest = (char*)arena_alloc(&wb->arena, (u32)word_len + 1);
        if (!dest) {
            FATAL("arena exhausted — this is a bug, bytes_needed was wrong");
        }

        memcpy(dest, json_buf + t->start, (size_t)word_len);
        dest[word_len] = '\0';

        // Store the offset, not the pointer
        genVec_push(&wb->words, cast(offset));
        words_found++;
    }

    // Clean up temporaries

    free(tokens);
    free(json_buf); // safe: all data is now in the arena

    LOG("loaded %u words (%llu bytes in arena)", words_found, (unsigned long long)arena_used(&wb->arena));
    return wb;
}

// wordbank_destroy

void wordbank_destroy(WordBank* wb)
{
    if (!wb) { return; }
    // arena_release(&wb->arena);   // release frees the struct
    free(wb->arena.base);
    genVec_destroy_stk(&wb->words); // words vec struct is inline in wb
    free(wb);
}

// wordbank_word_at
// Internal helper: look up the NUL-terminated C string for word index i.
// Returns a pointer directly into the arena

static const char* wordbank_word_at(WordBank* wb, u32 i)
{
    u32 offset = *(u32*)genVec_get_ptr(&wb->words, i);
    return (const char*)(wb->arena.base + offset);
}


// wordbank_random_word
//
// Returns a pointer into the arena. Valid for the lifetime of the WordBank.
// It is a plain C string (NUL-terminated)
const char* wordbank_random_word(WordBank* wb)
{
    CHECK_WARN_RET(!wb || wb->words.size == 0, NULL, "empty wordbank");
    u32 idx = pcg32_rand_bounded((u32)wb->words.size);
    return wordbank_word_at(wb, idx);
}

// wordbank_random_words
//
// Returns a genVec* of (u32) indices — NOT copies of the words.
// Use wordbank_word_at() or the macro below to get the actual string for each.
//
// Algorithm: Fisher-Yates partial shuffle on a temporary index array.
//   - Build indices[0..N-1].
//   - For i in [0, count): swap indices[i] with a random j in [i, N).
//   - The first `count` slots after the loop are our chosen indices.
//
// This is O(N) in the total wordbank size. If N is very large and count is
// small, a hash-set based reservoir sample would be faster, but for typical
// typing-test sizes (≤ 10 000 words, count ≤ 200) this is fine.

genVec* wordbank_random_words(WordBank* wb, u32 count)
{
    CHECK_WARN_RET(!wb, NULL, "null wordbank");

    u32 total = (u32)wb->words.size;
    if (count > total) {
        WARN("requested %u words but bank only has %u — clamping", count, total);
        count = total;
    }

    // Scratch index array on the heap
    u32* indices = malloc(total * sizeof(u32));
    CHECK_FATAL(!indices, "OOM in wordbank_random_words");

    for (u32 i = 0; i < total; i++) { indices[i] = i; }

    // Partial Fisher-Yates: only shuffle the first `count` positions.
    for (u32 i = 0; i < count; i++) {
        // j is a random index in [i, total)
        u32 j       = i + pcg32_rand_bounded(total - i);
        u32 tmp     = indices[i];
        indices[i]  = indices[j];
        indices[j]  = tmp;
    }

    // Build result vec of u32 word indices.
    genVec* result = genVec_init(count, sizeof(u32), NULL);
    for (u32 i = 0; i < count; i++) {
        genVec_push(result, (u8*)&indices[i]);
    }

    free(indices);
    return result;
}


