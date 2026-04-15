#include "json_wordbank_loader.h"

#include "gen_vector_single.h"
#include "jsmn.h"
#include "random_single.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct {
    FILE* f;
    u32   size_bytes;  // size of file in bytes
    u32   words_start; // offset to words array
    u32   words_end;   // offset to end of words array
    u32   num_words;   // number of words
} json_wordbank;



// static size_t get_file_size(FILE* f)
// {
//     fseek(f, 0, SEEK_END);
//     size_t size = (size_t)ftell(f);
//     fseek(f, 0, SEEK_SET);
//     return size;
// }


// Helper: check if token matches a string
static bool jsoneq(const char* json, jsmntok_t* tok, const char* s)
{
    return (tok->type == JSMN_STRING && strlen(s) == (size_t)(tok->end - tok->start) &&
            strncmp(json + tok->start, s, (size_t)(tok->end - tok->start)) == 0) != 0;
}

// TODO: when we have tokenised json, how to count the number of words?
// we need this info for accurate arena memory allocation (sizeof(json file) + sizeof('\0') * no of words)



static bool json_wordbank_create(json_wordbank* json_wb, const char* filename) 
{
    json_wb->f = fopen(filename, "r");

    // find size of file

    // find word array start and end

    // find number of words

}

static bool json_wordbank_destroy(json_wordbank* json_wb)
{
    fclose(json_wb->f);
}


// public api


WordBank* wordbank_create(const char* filename)
{

}



