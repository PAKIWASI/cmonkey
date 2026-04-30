#include "timer.h"
#include "wc_test.h"


static int test_timer_1(void)
{
    cmonkey_timer timer = {0};
    timer_begin(&timer, 60);

    while (1) {
        timer_tick(&timer);

        // work
        for (u32 i = 0; i < 10000; i++) {
            u32 j = i*i*i*i*i;
            j *= j;
        }

        float dt = timer_get_delta(&timer);

        timer_end_frame(&timer);
        timer_sleep(&timer);
        printf("FPS: %.1f  dt: %.4f\n", timer_get_fps(&timer), dt);
    }

    return 0;
}




extern void test_timer_suite(void)
{
    WC_SUITE("timer");

    WC_RUN(test_timer_1);
}
