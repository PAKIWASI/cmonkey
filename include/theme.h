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
} border_style_t;

static const char* BORDER_CHARS[4][6] = {
    /* tl      tr      bl      br      v    h   */
    { "┌",   "┐",   "└",   "┘",   "│", "─" },   /* sharp   */

    { "╭",   "╮",   "╰",   "╯",   "│", "─" },   /* rounded */

    { "┏",   "┓",   "┗",   "┛",   "┃", "━" },   /* bold    */

    { "╔",   "╗",   "╚",   "╝",   "║", "═" },   /* double  */
};


#endif // CMONKEY_THEME_H
