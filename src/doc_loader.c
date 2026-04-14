#include <stdio.h>
#include <string.h>
#include "jsmn.h"


static int jsoneq(const char *json, jsmntok_t *tok, const char *s) 
{
    if (tok->type == JSMN_STRING &&
        (int)strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

// TODO: instead of loading entire file into memory,
// why don't we just load random words (offsets)
// but that would have a performance penalty for user who takes
// multiple tests


static long file_size(FILE *f) {
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    return size;
}

int run() 
{
    const char *json = /* your JSON string here */;

    jsmn_parser parser;
    // TODO: dynamically allocate in genVec
    jsmntok_t tokens[2048];

    jsmn_init(&parser);
    int count = jsmn_parse(&parser, json, strlen(json), tokens, 2048);

    if (count < 0) {
        printf("Failed to parse JSON\n");
        return 1;
    }

    for (int i = 1; i < count; i++) {
        if (jsoneq(json, &tokens[i], "words") == 0) {

            jsmntok_t *array = &tokens[i + 1];

            if (array->type != JSMN_ARRAY) {
                printf("words is not an array\n");
                return 1;
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

    return 0;
}
