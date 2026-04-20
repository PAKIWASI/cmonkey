#ifndef CMONKEY_THEME_H
#define CMONKEY_THEME_H

#include "common_single.h"


typedef struct {
    u8 r, g, b; 
} rgb;


typedef struct {
    rgb fg, bg;        // defaults
    rgb accent;        // highlights, cursor
    rgb correct;       // correct keystrokes
    rgb wrong;         // wrong keystrokes
    rgb dim;           // ghost/upcoming text

    // border style: 0=sharp, 1=rounded, 2=bold
    int border_style;
} theme;


// border character sets indexed by style enum
typedef enum {
    BORDER_SHARP    = 0,
    BORDER_ROUNDED  = 1,
    BORDER_BOLD     = 2,
    BORDER_DOUBLE   = 3
} border_style;

static const char* BORDER_CHARS[4][6] = {
    /* tl      tr      bl      br      v    h   */
    { "┌",   "┐",   "└",   "┘",   "│", "─" },   /* sharp   */

    { "╭",   "╮",   "╰",   "╯",   "│", "─" },   /* rounded */

    { "┏",   "┓",   "┗",   "┛",   "┃", "━" },   /* bold    */

    { "╔",   "╗",   "╚",   "╝",   "║", "═" },   /* double  */
};


/* static inline: definition lives in the header, each TU gets its own copy.
   Plain 'inline' without 'static' requires an external definition elsewhere — 
   never use it in headers for functions with bodies.                          */

static inline rgb rgb_hex(uint32_t hex) {
    return (rgb){ (hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF };
}

static inline rgb rgb_lerp(rgb a, rgb b, float t) {
    return (rgb){
        (uint8_t)(a.r + (b.r - a.r) * t),
        (uint8_t)(a.g + (b.g - a.g) * t),
        (uint8_t)(a.b + (b.b - a.b) * t),
    };
}

static inline rgb rgb_dim(rgb c, float factor) {
    return (rgb){
        (uint8_t)(c.r * factor),
        (uint8_t)(c.g * factor),
        (uint8_t)(c.b * factor),
    };
}

#endif // CMONKEY_THEME_H
