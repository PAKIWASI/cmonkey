#ifndef JSON_WORDBANK_LOADER_H
#define JSON_WORDBANK_LOADER_H

#include "gen_vector_single.h"
#include "arena_single.h"


typedef struct {
    Arena  arena;  // owns all string data
    genVec words;  // vec of u32 byte-offsets into arena
} WordBank;


// Load entire JSON word list into memory.
WordBank*   wordbank_create(const char* filename);

// Destroy and free all memory.
void        wordbank_destroy(WordBank* wb);

// Get the C string for word at index i (pointer into arena)
const char* wordbank_word_at(WordBank* wb, u32 i);

// Get a single random word (pointer into arena).
const char* wordbank_random_word(WordBank* wb);

// Get a genVec* of u32 word-indices, randomly selected without replacement.
// Iterate with: *(u32*)genVec_get_ptr(v, i)  then pass to wordbank_word_at().
// Caller must genVec_destroy() the returned vec.
genVec*     wordbank_random_words(WordBank* wb, u32 count);

#endif // JSON_WORDBANK_LOADER_H
