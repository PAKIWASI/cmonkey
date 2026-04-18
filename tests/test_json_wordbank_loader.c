#include "gen_vector_single.h"
#include "wc_test.h"
#include "json_wordbank_loader.h"
#include <stdio.h>


#define FILE_PATH_ENG "/home/wasi/Documents/projects/c/cmonkey/data/english.json"
#define FILE_PATH_ENG1K "/home/wasi/Documents/projects/c/cmonkey/data/english_1k.json"
#define FILE_PATH_ENG5K "/home/wasi/Documents/projects/c/cmonkey/data/english_10k.json"
#define FILE_PATH_ENG10K "/home/wasi/Documents/projects/c/cmonkey/data/english_5k.json"
#define FILE_PATH_ENG25K "/home/wasi/Documents/projects/c/cmonkey/data/english_25k.json"
#define FILE_PATH_ENG450K "/home/wasi/Documents/projects/c/cmonkey/data/english_450k.json"
#define FILE_PATH_ENG_MISSPELLED "/home/wasi/Documents/projects/c/cmonkey/data/english_commonly_misspelled.json"
#define FILE_PATH_ARABIC "/home/wasi/Documents/projects/c/cmonkey/data/arabic.json"
#define FILE_PATH_C "/home/wasi/Documents/projects/c/cmonkey/data/code_c.json"

#define CURR_FILE FILE_PATH_ENG450K


static int test_wb_create(void)
{
    WordBank* wb = wordbank_create(CURR_FILE);
    WC_ASSERT(wb);

    WC_ASSERT_EQ_U64(genVec_size(wb->words), wordbank_size(wb));

    wordbank_destroy(wb);
    return 0;
}

static int test_get_random_words(void)
{
    WordBank* wb = wordbank_create(CURR_FILE);

    genVec* v;
    for (u32 i = 0; i < 100; i++) {
        v = wordbank_random_words(wb, 100);
        printf("%s\t", wordbank_word_at(wb, *(u32*)genVec_get_ptr(v, i)));
        WC_ASSERT(v);
        WC_ASSERT_EQ_U64(100, genVec_size(v));
        genVec_destroy(v);
    }

    v = wordbank_random_words(wb, 100000000);
    printf("%s\t", wordbank_word_at(wb, *(u32*)genVec_get_ptr(v, 10)));
    WC_ASSERT(v);
    // we only have 200 words i this wordbank
    WC_ASSERT_EQ_U64(wordbank_size(wb), genVec_size(v));

    genVec_destroy(v);
    wordbank_destroy(wb);
    return 0;
}

extern void json_file_suite(void)
{
    WC_SUITE("JSON File Read Tests");

    WC_RUN(test_wb_create);
    WC_RUN(test_get_random_words);
}



