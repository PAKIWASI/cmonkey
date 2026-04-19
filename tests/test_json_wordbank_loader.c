#include "wc_test.h"
#include "wordbank.h"
#include "gen_vector_single.h"

#include <stdio.h>

#define ENG "english.json"
#define ENG1K "english_1k.json"
#define ENG5K "english_10k.json"
#define ENG10K "english_5k.json"
#define ENG25K "english_25k.json"
#define ENG450K "english_450k.json"
#define ENG_MISSPELLED "english_commonly_misspelled.json"
#define ARABIC "arabic.json"
#define C "code_c.json"

#define FOLDER_PATH "wordbanks/"
#define CURR_FILE (FOLDER_PATH ENG450K)



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

    wordbank_destroy(wb);
    return 0;
}

static int test_get_random_words_stress(void)
{
    WordBank* wb = wordbank_create(CURR_FILE);

    genVec* v;
    for (u32 i = 0; i < 10000; i++) {
        v = wordbank_random_words(wb, 1000);
        WC_ASSERT(v);
        WC_ASSERT_EQ_U64(1000, genVec_size(v));
        genVec_destroy(v);

        if (i % 1000 == 0) {
            LOG("i: %d\n", i);
        }
    }

    wordbank_destroy(wb);
    return 0;
}

#include <locale.h>

extern void json_file_suite(void)
{
    setlocale(LC_ALL, "");

    WC_SUITE("JSON File Read Tests");

    WC_RUN(test_wb_create);
    WC_RUN(test_get_random_words);
    WC_RUN(test_get_random_words_stress);
}



