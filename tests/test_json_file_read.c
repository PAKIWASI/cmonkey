#include "wc_test.h"
#include "json_wordbank_loader.h"


#define FILE_PATH ("/home/wasi/Documents/projects/c/cmonkey/data/english.json")


static int test_wb_create(void)
{
    WordBank* wb = wordbank_create(FILE_PATH);
    WC_ASSERT(wb);
    wordbank_destroy(wb);
    return 0;
}


extern void json_file_suite(void)
{
    WC_SUITE("JSON File Read Tests");

    WC_RUN(test_wb_create);
}
