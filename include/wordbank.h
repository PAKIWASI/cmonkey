#ifndef CMONKEY_WORDBANK_H
#define CMONKEY_WORDBANK_H

#include "gen_vector_single.h"
#include "arena_single.h"


typedef struct {
    Arena*  arena;  // owns all string data
    genVec* words;  // vec of u32 byte-offsets into arena
} WordBank;

// TODO: 
// 1. should we limit to loading a maximum of 100K words per launch
//  we have english 450K and the launch is not smooth
// 2. ARABIC is loading correctly with setlocate(), idk about cursor movement

// Load entire JSON word list into memory.
WordBank* wordbank_create(const char* filename);

// Destroy and free all memory.
void wordbank_destroy(WordBank* wb);

// Get the C string for word at index i (pointer into arena)
static inline const char* wordbank_word_at(WordBank* wb, u32 i)
{
    u32 offset = *(u32*)genVec_get_ptr(wb->words, i);
    return (const char*)(wb->arena->base + offset);
}

// Get a single random word (pointer into arena).
const char* wordbank_random_word(WordBank* wb);

// Get a genVec* of u32 word-indices, randomly selected without replacement.
// Iterate with: *(u32*)genVec_get_ptr(v, i)  then pass to wordbank_word_at().
// Caller must genVec_destroy() the returned vec.
genVec* wordbank_random_words(WordBank* wb, u32 count);

static inline u64 wordbank_size(WordBank* wb) {
    return genVec_size(wb->words);
}

#endif // CMONKEY_WORDBANK_H
