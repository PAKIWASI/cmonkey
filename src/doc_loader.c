#include "jsmn.h"
#include "doc_loader.h"

#include <stdio.h>
#include <string.h>



static bool jsoneq(const char *json, jsmntok_t *tok, const char *s) 
{
    if (tok->type == JSMN_STRING &&
        strlen(s) == (u32)(tok->end - tok->start) &&
        strncmp(json + tok->start, s, (u32)(tok->end - tok->start)) == 0) {
        return true;
    }
    return false;
}

// TODO: instead of loading entire file into memory,
// why don't we just load random words (offsets)
// but that would have a performance penalty for user who takes multiple tests
// we can circumvent that by loading 2 tests at a time and when user doing 2nd test,
// we load 2 more tests in the background


static u32 file_size(FILE *f) 
{
    fseek(f, 0, SEEK_END);
    u32 size = (u32)ftell(f);
    fseek(f, 0, SEEK_SET);
    return size;
}


bool load_from_doc(const char* filename)
{
    FILE* f = fopen(filename, "r"); 
    if (!f) { return false; }

    u32 size_bytes = file_size(f);

    const char *json = /* your JSON string here */;

    jsmn_parser parser;
    // TODO: dynamically allocate in genVec
    jsmntok_t tokens[2048];

    jsmn_init(&parser);
    int count = jsmn_parse(&parser, json, strlen(json), tokens, 2048);

    if (count < 0) {
        printf("Failed to parse JSON\n");
        return false;
    }

    // TODO: how to find number of items in words arr ?
    for (int i = 1; i < count; i++) 
    {
        if (jsoneq(json, &tokens[i], "words")) 
        {
            jsmntok_t *array = &tokens[i + 1];

            if (array->type != JSMN_ARRAY) {
                printf("words is not an array\n");
                return false;
            }

            int idx = i + 2;

            for (int j = 0; j < array->size; j++) {
                jsmntok_t *word = &tokens[idx + j];

                printf("%.*s\n",
                       word->end - word->start,
                       json + word->start);
            }

            break;
        }
    }

    fclose(f);
    return true;
}

