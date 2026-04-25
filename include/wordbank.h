#ifndef CMONKEY_WORDBANK_H
#define CMONKEY_WORDBANK_H

#include "Queue_single.h"
#include "arena_single.h"


typedef struct {
    u32 idx;    // offset into arena if all words inline
    u32 len;    // length of the word
} Word;

typedef struct {
    Arena*  arena;      // owns all string data
    genVec* words;      // vec of Word type
    u32*    scratch;    // pre-allocated index array for random selection (size == words->size)
    u32*    swapped_j;  // tracks what indices were swapped, to swap them back afterwards
    u32     num_random_words;   // how many words(idx) user will want each call
} WordBank;

// TODO: 
// 2. ARABIC is loading correctly with setlocate(), idk about cursor movement

// Load entire JSON word list into memory.
WordBank* wordbank_create(const char* filename, u32 num_random_words);

// Destroy and free all memory.
void wordbank_destroy(WordBank* wb);

// TODO:
void wordbank_switch(WordBank* wb, const char* filename);

// Get the C string for word at index i (pointer into arena)
static inline const char* wordbank_word_at(WordBank* wb, u32 i)
{
    return (const char*)(wb->arena->base + i);
}

// Partial Fisher-Yates: O(N) time, my modified verion: O(buff_size)
void wordbank_random_words(WordBank* wb, u32* buff, u32 buff_size);


/*
    we push 'num_random_words' elm indexes to back of queue
    queue is a subset of words genvec, words should be accessed by
    calling wordbank_word_at() with the index from front of queue
    if queue is exausted, we load more at the back
*/
void wordbank_random_words_in_queue(WordBank* wb, Queue* q);


static inline u64 wordbank_size(WordBank* wb) {
    return genVec_size(wb->words);
}

#endif // CMONKEY_WORDBANK_H
