#include "buffer.h"
#include "config.h"
#include "draw.h"
#include "wc_test.h"

#include <stdio.h>
#include <unistd.h>

#define THEME_PATH  "config/tokyonight.theme"
#define CONF_PATH   "config/cmonkey.conf"


static int test_box(void)
{
    cmonkey_theme t = {0};
    cmonkey_conf  c = {0};

    theme_load(&t, THEME_PATH);
    config_load(&c, CONF_PATH);

    term_buf b;
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

    theme_load(&t, THEME_PATH);
    config_load(&c, CONF_PATH);

    term_buf b;
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


extern void config_draw_suite(void)
{
    WC_SUITE("box");
    WC_RUN(test_box);
    WC_RUN(test_box_text);

}


