#ifndef CMONEKEY_CONF_H
#define CMONEKEY_CONF_H

#include "common_single.h"
#include "theme.h"



typedef struct {
    u8    border_style;          // move this out of here and in config
    u8    cursor_style;          // block/bar/underline/none
    u8    cursor_trail_len;      // 0 = disabled, max 4
    float cursor_trail_decay_ms; // how fast trail fades to bg
} cmonkey_conf;


typedef enum {
    CURSOR_BLOCK     = 0,   // default
    CURSOR_BAR       = 1,
    CURSOR_UNDERLINE = 2,
    CURSOR_NONE      = 3,
} cursor_style;

// border character sets indexed by style enum
typedef enum {
    BORDER_SHARP   = 0,
    BORDER_ROUNDED = 1,     // default
    BORDER_BOLD    = 2,
    BORDER_DOUBLE  = 3,
} border_style;


static const char* BORDER_CHARS[4][6] = {
    /* tl      tr      bl      br      v    h   */
    { "┌",   "┐",   "└",   "┘",   "│", "─" },   /* sharp   */

    { "╭",   "╮",   "╰",   "╯",   "│", "─" },   /* rounded */

    { "┏",   "┓",   "┗",   "┛",   "┃", "━" },   /* bold    */

    { "╔",   "╗",   "╚",   "╝",   "║", "═" },   /* double  */
};


bool cmonkey_import_conf(cmonkey_conf* conf, const char* confpath);
bool cmonkey_import_theme(cmonkey_theme* t, const char* themepath);

#endif // CMONEKEY_CONF_H
