#include "buffer.h"
#include "config.h"
#include "draw.h"
#include "wc_test.h"

#include <unistd.h>

// ── helpers ──────────────────────────────────────────────────────────────────

#define THEME_PATH  "config/dracula.theme"
#define CONF_PATH   "config/cmonkey.conf"

// Pause briefly between visual tests so the tester can read the output
#define VISUAL_PAUSE() sleep(1)


// ── theme loading ─────────────────────────────────────────────────────────────

/*
 * theme_load_fields — load the dracula theme and assert every field that
 * the file defines was actually populated (not an empty string).
 */
static int test_theme_load_fields(void)
{
    cmonkey_theme t = {0};
    theme_load(&t, THEME_PATH);

    // Fields that have explicit values in dracula.theme
    WC_ASSERT(t.main_fg[0]   != '\0');
    WC_ASSERT(t.main_bg[0]   != '\0');
    WC_ASSERT(t.border[0]    != '\0');
    WC_ASSERT(t.cursor[0]    != '\0');
    WC_ASSERT(t.text_dim[0]  != '\0');
    WC_ASSERT(t.correct[0]   != '\0');
    WC_ASSERT(t.incorrect[0] != '\0');

    // Fields intentionally left empty in the file — must stay empty
    WC_ASSERT(t.text_fg[0]   == '\0');
    WC_ASSERT(t.text_bg[0]   == '\0');

    return 0;
}

/*
 * theme_load_reset — the reset field must start with the hard-reset code
 * and then embed main_fg and main_bg.
 */
static int test_theme_load_reset(void)
{
    cmonkey_theme t = {0};
    theme_load(&t, THEME_PATH);

    WC_ASSERT(strncmp(t.reset, "\033[0m", 4) == 0);

    // reset must contain the main_fg escape somewhere after the hard reset
    WC_ASSERT(strstr(t.reset, t.main_fg) != NULL);
    WC_ASSERT(strstr(t.reset, t.main_bg) != NULL);

    return 0;
}

/*
 * theme_escape_format — every populated colour field must match the
 * pattern "\033[38;2;" or "\033[48;2;" followed by digits.
 */
static int test_theme_escape_format(void)
{
    cmonkey_theme t = {0};
    theme_load(&t, THEME_PATH);

    const char* fg_fields[] = {
        t.main_fg, t.border, t.cursor, t.text_dim, t.correct, t.incorrect
    };
    const char* bg_fields[] = {
        t.main_bg
    };

    for (u32 i = 0; i < sizeof(fg_fields)/sizeof(fg_fields[0]); i++) {
        WC_ASSERT(strncmp(fg_fields[i], "\033[38;2;", 7) == 0);
    }
    for (u32 i = 0; i < sizeof(bg_fields)/sizeof(bg_fields[0]); i++) {
        WC_ASSERT(strncmp(bg_fields[i], "\033[48;2;", 7) == 0);
    }

    return 0;
}

/*
 * theme_unknown_path — loading a nonexistent file must not crash and must
 * leave the struct zeroed (all fields empty).
 */
static int test_theme_unknown_path(void)
{
    cmonkey_theme t = {0};
    theme_load(&t, "/nonexistent/path/to/theme.theme");

    WC_ASSERT(t.main_fg[0] == '\0');
    WC_ASSERT(t.reset[0]   == '\0');

    return 0;
}


// ── conf loading ──────────────────────────────────────────────────────────────

/*
 * conf_load_values — load cmonkey.conf and assert the parsed enum values
 * match what the file says ("rounded", "bar").
 */
static int test_conf_load_values(void)
{
    cmonkey_conf c = {0};
    config_load(&c, CONF_PATH);

    WC_ASSERT(c.border_style == BORDER_ROUND);
    WC_ASSERT(c.cursor_style == CURSOR_BAR);

    return 0;
}

/*
 * conf_unknown_path — loading a nonexistent conf must not crash.
 */
static int test_conf_unknown_path(void)
{
    cmonkey_conf c = {0};
    config_load(&c, "/nonexistent/cmonkey.conf");
    // If we're here, no crash — defaults stay at 0 (BORDER_SHARP, CURSOR_BAR)
    WC_ASSERT(c.border_style == BORDER_SHARP);
    return 0;
}


// ── visual / draw tests ───────────────────────────────────────────────────────

/*
 * visual_theme_swatches — render one labelled colour swatch per theme field.
 * Each swatch is a row of '█' printed in the theme colour against the main_bg,
 * with the field name in plain text to the right.
 *
 * Expected: you see 7 coloured bars with matching labels.
 */
static int test_visual_theme_swatches(void)
{
    cmonkey_theme t = {0};
    theme_load(&t, THEME_PATH);

    term_buf b;
    tb_create(&b, 40, 80);

    // Clear + apply theme base
    draw_clear(&b, &t);

    struct { const char* label; const char* fg; const char* bg; } swatches[] = {
        { "main_fg",   t.main_fg,   t.main_bg  },
        { "main_bg",   t.main_fg,   t.main_bg  },  // show bg as a bg swatch
        { "border",    t.border,    t.main_bg  },
        { "cursor",    t.cursor,    t.main_bg  },
        { "text_dim",  t.text_dim,  t.main_bg  },
        { "correct",   t.correct,   t.main_bg  },
        { "incorrect", t.incorrect, t.main_bg  },
    };

    u32 num = sizeof(swatches) / sizeof(swatches[0]);
    for (u32 i = 0; i < num; i++) {
        u32 row = 3 + (i * 2);

        // Coloured bar
        draw_color_swatch(&b, row, 4, 12,
                          swatches[i].fg, swatches[i].bg, (char)0xe2); // '█' multi-byte; use ' ' fallback
        // For ASCII-safe terminals, just fill with spaces on the bg:
        draw_move(&b, row, 4);
        if (swatches[i].fg[0]) { draw_fg(&b, swatches[i].fg); }
        if (swatches[i].bg[0]) { draw_bg(&b, swatches[i].bg); }
        tb_append_cstr(&b, "            "); // 12 spaces
        draw_reset(&b);

        // Label in main_fg
        draw_text_at(&b, row, 18, t.main_fg, &t, swatches[i].label);
    }

    // Footer
    draw_text_at(&b, 3 + (num * 2) + 1, 4, t.text_dim, &t,
                 "-- press enter --");

    tb_flush(&b);
    tb_destroy(&b);

    // Wait for tester to observe the output
    getchar();
    return 0;
}

/*
 * visual_text_attributes — render the same word in every text attribute
 * combination that draw.h exposes.
 */
static int test_visual_text_attributes(void)
{
    cmonkey_theme t = {0};
    theme_load(&t, THEME_PATH);

    term_buf b;
    tb_create(&b, 40, 80);

    draw_clear(&b, &t);
    draw_fg(&b, t.main_fg);

    struct {
        const char* label;
        void (*on)(term_buf*);
        void (*off)(term_buf*);
    } attrs[] = {
        { "bold",      draw_bold_on,      draw_bold_off      },
        { "dim",       draw_dim_on,       draw_dim_off       },
        { "italic",    draw_italic_on,    draw_italic_off    },
        { "underline", draw_underline_on, draw_underline_off },
        { "strike",    draw_strike_on,    draw_strike_off    },
    };

    u32 num = sizeof(attrs) / sizeof(attrs[0]);
    for (u32 i = 0; i < num; i++) {
        u32 row = 3 + (i * 2);
        draw_move(&b, row, 4);
        draw_fg(&b, t.main_fg);

        attrs[i].on(&b);
        tb_append_cstr(&b, "The quick brown fox");
        attrs[i].off(&b);

        // label beside it in dim
        draw_text_at(&b, row, 26, t.text_dim, &t,attrs[i].label);
    }

    draw_text_at(&b, 3 + (num * 2) + 1, 4, t.text_dim, &t,"-- press enter --");

    tb_flush(&b);
    tb_destroy(&b);

    getchar();
    return 0;
}

/*
 * visual_correct_incorrect — simulate a word with mixed correct / incorrect
 * characters, as the typing test would render them.
 */
static int test_visual_correct_incorrect(void)
{
    cmonkey_theme t = {0};
    theme_load(&t, THEME_PATH);

    term_buf b;
    tb_create(&b, 10, 80);

    draw_clear(&b, &t);

    const char* word    = "example";
    const char* typed   = "exXmplZ";   // positions 2 and 6 are wrong

    draw_move(&b, 5, 10);
    for (u32 i = 0; word[i]; i++) {
        if (typed[i] == word[i]) {
            draw_fg(&b, t.correct);
        } else {
            draw_fg(&b, t.incorrect);
        }
        tb_append_n(&b, &word[i], 1);
    }
    draw_reset(&b);

    // legend
    draw_text_at(&b, 7, 10, t.correct,  &t ,"correct colour");
    draw_text_at(&b, 8, 10, t.incorrect, &t,"incorrect colour");
    draw_text_at(&b, 10, 10, t.text_dim, &t,"-- press enter --");

    tb_flush(&b);
    tb_destroy(&b);

    getchar();
    return 0;
}

/*
 * visual_theme_reset — verify that draw_theme_reset actually restores the
 * theme colours after a colour change mid-buffer.
 */
static int test_visual_theme_reset(void)
{
    cmonkey_theme t = {0};
    theme_load(&t, THEME_PATH);

    term_buf b;
    tb_create(&b, 10, 80);

    draw_clear(&b, &t);

    // Print something in "incorrect" (red)
    draw_text_at(&b, 4, 4, t.incorrect, &t,"this is red   ");

    // Reset back to theme
    draw_theme_reset(&b, &t);

    // This should be back in main_fg
    draw_move(&b, 5, 4);
    tb_append_cstr(&b, "back to normal  ");

    draw_text_at(&b, 7, 4, t.text_dim, &t,"-- press enter --");

    tb_flush(&b);
    tb_destroy(&b);

    getchar();
    return 0;
}

static int test_box(void)
{
    cmonkey_theme t = {0};
    cmonkey_conf  c = {0};

    theme_load(&t, THEME_PATH);
    config_load(&c, CONF_PATH);

    term_buf b;
    tb_create(&b, 40, 80);

    draw_box_at(&b, 0, 0, 10, 10, &t, &c);

    tb_flush(&b);
    tb_destroy(&b);
    return 0;
}


// ── suite entry point ─────────────────────────────────────────────────────────

extern void config_draw_suite(void)
{
    // WC_SUITE("config / theme loading");
    // WC_RUN(test_theme_load_fields);
    // WC_RUN(test_theme_load_reset);
    // WC_RUN(test_theme_escape_format);
    // WC_RUN(test_theme_unknown_path);
    //
    // WC_SUITE("conf loading");
    // WC_RUN(test_conf_load_values);
    // WC_RUN(test_conf_unknown_path);
    //
    // WC_SUITE("visual — theme colours (interactive)");
    // WC_RUN(test_visual_theme_swatches);
    // WC_RUN(test_visual_text_attributes);
    // WC_RUN(test_visual_correct_incorrect);
    // WC_RUN(test_visual_theme_reset);
    //
    // WC_SUITE("box");
    WC_RUN(test_box);
}


