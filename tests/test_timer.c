#include "timer.h"
#include "wc_test.h"


static int test_timer_1(void)
{
    cmonkey_timer timer = {0};
    timer_begin(&timer, 60);

    while (1) {
        timer_tick(&timer);

        // work
        for (u32 i = 0; i < 1000; i++) {
            for (u32 k = 0; k < 10000; k++) {
                u32 j = i*i*i*i*i;
                j *= j;
            }
        }


        timer_end_frame(&timer);
        float dt = timer_get_delta(&timer);
        float fps = timer_get_fps(&timer);
        timer_sleep(&timer);
        printf("FPS: %.1f  dt: %.4f\n", fps, dt);
    }

    return 0;
}




extern void test_timer_suite(void)
{
    WC_SUITE("timer");

    WC_RUN(test_timer_1);
}
