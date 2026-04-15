#ifndef DOC_LOADER_H
#define DOC_LOADER_H

#include "gen_vector_single.h"

/*
    HOW TO DO IT

- Store the selected words in a const cstr buffer
- index it using a genVec of cstr pointers
- the buffer's size can be determined by the doc size?
    but we need to account for '\0' (equal to num of words)
- any way to find the number of words?
*/


typedef struct {
    const char* ptr;
    u32 size_bytes;
    u32 num_words;
} Doc;


bool load_from_doc(const char* filename);



#endif // DOC_LOADER_H
