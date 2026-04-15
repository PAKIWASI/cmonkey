#ifndef JSON_WORDBANK_LOADER_H
#define JSON_WORDBANK_LOADER_H

#include "gen_vector_single.h"
#include "arena_single.h"


typedef struct {
    Arena arena;       // owns all string data
    genVec words;      // vec of index offsets into arena
} WordBank;


// Load entire JSON word list into memory
WordBank* wordbank_create(const char* filename);

// Destroy and free all memory
void wordbank_destroy(WordBank* wb);

// Get a single random word
const char* wordbank_random_word(WordBank* wb);

// Get N random unique words
// Words are selected without replacement
genVec* wordbank_random_words(WordBank* wb, u32 count);


#endif // JSON_WORDBANK_LOADER_H
