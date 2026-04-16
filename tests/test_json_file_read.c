#include "gen_vector_single.h"
#include "wc_test.h"
#include "json_wordbank_loader.h"


#define FILE_PATH_ENG ("/home/wasi/Documents/projects/c/cmonkey/data/english.json")


static int test_wb_create(void)
{
    WordBank* wb = wordbank_create(FILE_PATH_ENG);
    WC_ASSERT(wb);

    WC_ASSERT_EQ_U64(genVec_size(wb->words), 200);

    wordbank_destroy(wb);
    return 0;
}


extern void json_file_suite(void)
{
    WC_SUITE("JSON File Read Tests");

    WC_RUN(test_wb_create);
}
