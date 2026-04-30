#include "Queue_single.h"
#include "buffer.h"
#include "config.h"
#include "draw.h"
#include "wc_test.h"
#include "wordbank.h"

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>



#define ENG         "english_450k.json"
#define FOLDER_PATH     "wordbanks/"
#define CURR_FILE       (FOLDER_PATH ENG)

#define NUM_RAND_WORDS  200

#define THEME_PATH  "config/tokyonight.theme"
#define CONF_PATH   "config/cmonkey.conf"


static int test_box(void)
{
    cmonkey_theme t = {0};
    cmonkey_conf  c = {0};
    term_buf      b = {0};

    theme_load(&t, THEME_PATH);
    config_load(&c, CONF_PATH);

    tb_create(&b, 40, 80);

    draw_clear(&b, &t);

    draw_box(&b, (Box){ 10, 55, 10, 30 }, &t, &c);
    draw_box(&b, (Box){ 12, 57, 10, 30 }, &t, &c);
    draw_box(&b, (Box){ 14, 59, 10, 30 }, &t, &c);
    draw_box(&b, (Box){ 16, 61, 10, 30 }, &t, &c);

    tb_flush(&b);
    getchar();
    tb_destroy(&b);
    return 0;
}

static int test_box_text(void)
{
    cmonkey_theme t = {0};
    cmonkey_conf  c = {0};
    term_buf      b = {0};

    theme_load(&t, THEME_PATH);
    config_load(&c, CONF_PATH);

    tb_create(&b, 40, 80);

    draw_clear(&b, &t);

    draw_box(&b, (Box){ 1, 1, 70, 70 }, &t, &c);

    draw_text_with_color(&b, 2, 2, t.text_fg, &t,
    "hello my name is wasi ullah satti. cmonkey is coming along nicely!");

    tb_flush(&b);
    getchar();
    tb_destroy(&b);
    return 0;
}

static int test_words_in_box(void)
{
    cmonkey_theme t = {0};
    cmonkey_conf  c = {0};
    term_buf      b = {0};
    WordBank     wb = {0};
    Queue         q = {0};

    theme_load(&t, THEME_PATH);
    config_load(&c, CONF_PATH);

    tb_create(&b, 40, 80);

    wordbank_create(&wb, CURR_FILE, NUM_RAND_WORDS);
    queue_create_stk(&q, NUM_RAND_WORDS, sizeof(u32), NULL);

    wordbank_random_words_in_queue(&wb, &q);

    draw_clear(&b, &t);

    Box box = {1, 1, 40, 80 };

    draw_box(&b, box, &t, &c);

    draw_words_in_box(&b, box, &q, &wb, 200, &t);

    tb_flush(&b);
    getchar();
    tb_destroy(&b);
    wordbank_destroy(&wb);
    queue_destroy(&q);
    return 0;
}


extern void config_draw_suite(void)
{
    WC_SUITE("box");
    WC_RUN(test_box);
    WC_RUN(test_box_text);
    WC_RUN(test_words_in_box);

}


