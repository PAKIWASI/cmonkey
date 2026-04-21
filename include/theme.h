#ifndef CMONKEY_THEME_H
#define CMONKEY_THEME_H

#include "common_single.h"


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
// color + how to draw it
typedef struct {
    rgb  fg;
    rgb  bg;
    u8   attrs;       // bitmask of ATTR_* above
    bool has_bg;      // false = skip bg (inherit terminal)
} color_role;

typedef struct {
    char name[64];

    color_role base;
    color_role accent;
    color_role main_text;
    color_role correct;
    color_role wrong;
    color_role dim;
    color_role warning;    // low time, error states
    color_role cursor;
} cmonkey_theme;


static const char* BORDER_CHARS[4][6] = {
    /* tl      tr      bl      br      v    h   */
    { "┌",   "┐",   "└",   "┘",   "│", "─" },   /* sharp   */

    { "╭",   "╮",   "╰",   "╯",   "│", "─" },   /* rounded */

    { "┏",   "┓",   "┗",   "┛",   "┃", "━" },   /* bold    */

    { "╔",   "╗",   "╚",   "╝",   "║", "═" },   /* double  */
};



static inline rgb rgb_hex(uint32_t hex) {
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

#endif // CMONKEY_THEME_H
