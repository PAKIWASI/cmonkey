#define WC_TEST_MAIN
#include "wc_test.h"


void json_file_suite(void);
void buf_draw_suite(void);


int main(void)
{
    // json_file_suite();
    buf_draw_suite();

    return WC_REPORT();
}
