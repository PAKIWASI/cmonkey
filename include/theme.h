#ifndef CMONKEY_THEME_H
#define CMONKEY_THEME_H

#include "common_single.h"
#include <string.h>


typedef struct {
    u8 r, g, b;
} rgb;

// attrs bitmask — maps directly to draw_* calls
#define ATTR_BOLD      (1 << 0)
#define ATTR_ITALIC    (1 << 1)
#define ATTR_UNDERLINE (1 << 2)
#define ATTR_DIM       (1 << 3)
#define ATTR_STRIKE    (1 << 4)

// one semantic color slot with all its styling attached
typedef struct {
    rgb  fg;
    rgb  bg;
    u8   attrs;     // bitmask of ATTR_* above
    bool has_bg;    // false = skip bg (inherit terminal)
} color_role;

// ---------- enums live here so theme.h is self-contained ----------

typedef enum {
    CURSOR_BLOCK     = 0,
    CURSOR_BAR       = 1,
    CURSOR_UNDERLINE = 2,
    CURSOR_NONE      = 3,
} cursor_style;

typedef enum {
    BORDER_SHARP   = 0,
    BORDER_ROUNDED = 1,
    BORDER_BOLD    = 2,
    BORDER_DOUBLE  = 3,
} border_style;

// -----------------------------------------------------------------

typedef struct {
    char name[64];

    color_role base;        // bg + general fg
    color_role accent;      // highlights, box titles
    color_role main_text;   // neutral typing area text
    color_role correct;     // correctly typed chars
    color_role wrong;       // incorrectly typed chars
    color_role dim;         // upcoming/ghost text
    color_role warning;     // low time, error states
    color_role cursor;      // cursor color

    u8 border_style;
    u8 cursor_style;
} cmonkey_theme;


static const char* BORDER_CHARS[4][6] = {
    /* tl    tr    bl    br    v     h    */
    { "┌", "┐", "└", "┘", "│", "─" },   /* sharp   */

    { "╭", "╮", "╰", "╯", "│", "─" },   /* rounded */

    { "┏", "┓", "┗", "┛", "┃", "━" },   /* bold    */

    { "╔", "╗", "╚", "╝", "║", "═" },   /* double  */
};


// Built-in fallback — used when no theme file is found
static inline cmonkey_theme theme_default(void)
{
    cmonkey_theme t = {0};

    t.base.fg      = (rgb){0xf8, 0xf8, 0xf2};
    t.base.bg      = (rgb){0x28, 0x2a, 0x36};
    t.base.has_bg  = true;

    t.main_text.fg = (rgb){0xf8, 0xf8, 0xf2};
    t.accent.fg    = (rgb){0xbd, 0x93, 0xf9};
    t.correct.fg   = (rgb){0x50, 0xfa, 0x7b};
    t.wrong.fg     = (rgb){0xff, 0x55, 0x55};
    t.dim.fg       = (rgb){0x44, 0x47, 0x5a};
    t.warning.fg   = (rgb){0xff, 0xb8, 0x6c};
    t.cursor.fg    = (rgb){0xf8, 0xf8, 0xf2};

    t.border_style = BORDER_ROUNDED;
    t.cursor_style = CURSOR_BAR;

    strncpy(t.name, "default", sizeof(t.name) - 1);
    return t;
}


static inline rgb rgb_hex(u32 hex) {
    return (rgb){ (hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF };
}

static inline rgb rgb_lerp(rgb a, rgb b, float t) {
    return (rgb){
        (u8)((float)a.r + ((float)(b.r - a.r) * t)),
        (u8)((float)a.g + ((float)(b.g - a.g) * t)),
        (u8)((float)a.b + ((float)(b.b - a.b) * t)),
    };
}

static inline rgb rgb_dim(rgb c, float factor) {
    return (rgb){
        (u8)((float)c.r * factor),
        (u8)((float)c.g * factor),
        (u8)((float)c.b * factor),
    };
}

// average of two colors
static inline rgb rgb_mix(rgb a, rgb b) {
    return (rgb){
        (u8)(((u16)a.r + b.r) / 2),
        (u8)(((u16)a.g + b.g) / 2),
        (u8)(((u16)a.b + b.b) / 2),
    };
}

#endif // CMONKEY_THEME_H
