#include "cmonkey.h"
#include "Queue_single.h"
#include "wc_macros_single.h"
#include "wc_test.h"
#include "wordbank.h"

#include <stdio.h>
#include <string.h>


#define ENG "english.json"
#define ENG1K "english_1k.json"
#define ENG5K "english_10k.json"
#define ENG10K "english_5k.json"
#define ENG25K "english_25k.json"
#define ENG450K "english_450k.json"
#define ENG_MISSPELLED "english_commonly_misspelled.json"
#define ARABIC "arabic.json"
#define C "code_c.json"
#define BULLSHIT "bullshit.json"

#define FOLDER_PATH "wordbanks/"
#define CURR_FILE (FOLDER_PATH ENG1K)

#define NUM_RAND_WORDS 200


static int test_wb_create(void)
{
    WordBank* wb = wordbank_create(CURR_FILE, NUM_RAND_WORDS);
    WC_ASSERT_NOT_NULL(wb);
    if (!wb) {
        return -1;
    }

    WC_ASSERT_EQ_U64(genVec_size(wb->words), wordbank_size(wb));

    wordbank_destroy(wb);
    return 0;
}

static int test_get_random_words(void)
{
    WordBank* wb = wordbank_create(CURR_FILE, NUM_RAND_WORDS);
    WC_ASSERT(wb);
    if (!wb) {
        return -1;
    }

    u32 buff[NUM_RAND_WORDS] = {0};

    for (u32 i = 0; i < 10000; i++) {
        wordbank_random_words(wb, buff, NUM_RAND_WORDS);
        // printf("%s\t", wordbank_word_at(wb, buff[0]));
    }

    wordbank_destroy(wb);
    return 0;
}

static int test_get_words_in_queue(void)
{
    cmonkey cm = {0};
    cmonkey_init(&cm, NULL, NULL, CURR_FILE);

    cmonkey_more_words(&cm);

    while (!queue_empty(cm.test.words)) {
        printf("%s\t", wordbank_word_at(
            cm.wordbank, DEQUEUE(cm.test.words, u32)));
    }

    cmonkey_end(&cm);
    return 0;
}


#include <locale.h>

extern void json_file_suite(void)
{
    setlocale(LC_ALL, "");

    WC_SUITE("JSON File Read Tests");

    WC_RUN(test_wb_create);
    WC_RUN(test_get_random_words);
    WC_RUN(test_get_words_in_queue);
}



