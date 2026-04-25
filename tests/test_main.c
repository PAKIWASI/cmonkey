#define WC_TEST_MAIN
#include "wc_test.h"


void json_file_suite(void);
void config_draw_suite(void);


int main(void)
{
    json_file_suite();
    // config_draw_suite();

    return WC_REPORT();
}
