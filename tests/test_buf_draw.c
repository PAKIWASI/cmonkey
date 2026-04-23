
#include "buffer.h"
#include "draw.h"
#include "wc_test.h"



static int term_buf_draw(void)
{
    term_buf b;
    tb_create(&b, 80, 40);
    
    while (1) {
        draw_move(&b, 20, 20);
        draw_bold_on(&b);
        draw_word(&b, "hello");
        draw_bold_off(&b);
        draw_word(&b, "hello");

        tb_flush(&b);
    }




    tb_destroy(&b);
    return 0;
}


extern void buf_draw_suite(void)
{
    WC_SUITE("draw on buf and flush");

    WC_RUN(term_buf_draw);
}
