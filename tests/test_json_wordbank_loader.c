#include "Queue_single.h"
#include "wc_macros_single.h"
#include "wc_test.h"
#include "wordbank.h"

#include <stdio.h>
#include <unistd.h>

#define ENG             "english.json"
#define ENG1K           "english_1k.json"
#define ENG5K           "english_5k.json"
#define ENG10K          "english_10k.json"
#define ENG25K          "english_25k.json"
#define ENG450K         "english_450k.json"
#define ENG_MISSPELLED  "english_commonly_misspelled.json"
#define ARABIC          "arabic.json"
#define C               "code_c.json"
#define BULLSHIT        "bullshit.json"

#define FOLDER_PATH     "wordbanks/"
#define CURR_FILE       (FOLDER_PATH ENG450K)

#define NUM_RAND_WORDS  200



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

static int test_get_words_in_queue(void)
{
    WordBank* wb = wordbank_create(CURR_FILE, NUM_RAND_WORDS);
    Queue* q = queue_create(NUM_RAND_WORDS, sizeof(u32), NULL);

    wordbank_random_words_in_queue(wb, q);

    for (u32 i = 0; i < 210; i++) {
        u32 i1 = DEQUEUE(q, u32);   // queue stores index into genvec
        Word* w = (Word*)genVec_get_ptr(wb->words, i1); // use it to access Word struct (cont. index, len into arena)
        printf("%s\t", wordbank_word_at(wb, w->idx));   // use htat index to get the word
        printf("%d\t", w->len);         // also get len
    }

    queue_destroy(q);
    wordbank_destroy(wb);
    return 0;
}

static int test_get_words_refresh(void)
{
    WordBank* wb = wordbank_create(CURR_FILE, NUM_RAND_WORDS);
    Queue* q = queue_create((u64)NUM_RAND_WORDS * 2, sizeof(u32), NULL);

    wordbank_random_words_in_queue(wb, q);

    for (u32 i = 0; i < 1000; i++)
    {
        // refresh condition
        if ((float)queue_size(q) / (float)NUM_RAND_WORDS < 0.2F) {
            wordbank_random_words_in_queue(wb, q);
            LOG("queue refreshed: %lu", queue_size(q)); 
        }

        u32 i1 = DEQUEUE(q, u32);   // queue stores index into genvec
        Word* w = (Word*)genVec_get_ptr(wb->words, i1); // use it to access Word struct (cont. index, len into arena)
        printf("%s\t", wordbank_word_at(wb, w->idx));   // use htat index to get the word
        printf("%d\t", w->len);         // also get len
    }

    queue_destroy(q);
    wordbank_destroy(wb);
    return 0;
}

extern void json_file_suite(void)
{
    WC_SUITE("JSON File Read Tests");

    WC_RUN(test_wb_create);
    WC_RUN(test_get_words_in_queue);
    WC_RUN(test_get_words_refresh);
}


