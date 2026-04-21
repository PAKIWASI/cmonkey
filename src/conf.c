#include "conf.h"

#include <string.h>


static rgb rgb_hex_str(const char* s);
static border_style parse_border_style(const char* s);
static cursor_style parse_cursor_style(const char* s);


bool cmonkey_import_conf(cmonkey_conf* conf, const char* confpath)
{
    FILE* f = fopen(confpath, "r");
    if (!f) {
        return false;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        // skip comments and blank lines
        char* p = line;
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == '#' || *p == '\n' || *p == '\0') {
            continue;
        }

        // match conf[key]="string"
        // TODO: or conf[key]=number
        char key[64], val[64];
        if (sscanf(p, "conf[%63[^]]]=\"%63[^\"]\"", key, val) != 2) {
            continue;
        }

        if (strcmp(key, "border_style") == 0) {

        } else if (strcmp(key, "cursor_style") == 0) {
        } else if (strcmp(key, "trail_len") == 0) {
        } else if (strcmp(key, "trail_decay_ms") == 0) {
        }
        // unknown keys: silently ignore — forward compat
    }

    fclose(f);
    return true;
}

bool cmonkey_import_theme(cmonkey_theme* t, const char* themepath)
{
    FILE* f = fopen(themepath, "r");
    if (!f) {
        return false;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        // skip comments and blank lines
        char* p = line;
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == '#' || *p == '\n' || *p == '\0') {
            continue;
        }

        // match theme[key]="value"
        char key[64], val[64];
        if (sscanf(p, "theme[%63[^]]]=\"%63[^\"]\"", key, val) != 2) {
            continue;
        }

        if (strcmp(key, "bg") == 0) {
            t->base.bg = rgb_hex_str(val);
        } else if (strcmp(key, "fg") == 0) {
            t->base.fg = rgb_hex_str(val);
        } else if (strcmp(key, "correct") == 0) {
            t->correct.fg = rgb_hex_str(val);
        } else if (strcmp(key, "wrong") == 0) {
            t->wrong.fg = rgb_hex_str(val);
        } else if (strcmp(key, "dim") == 0) {
            t->dim.fg = rgb_hex_str(val);
        } else if (strcmp(key, "accent") == 0) {
            t->accent.fg = rgb_hex_str(val);
        } else if (strcmp(key, "cursor") == 0) {
            t->cursor.fg = rgb_hex_str(val);
        } else if (strcmp(key, "warning") == 0) {
            t->warning.fg = rgb_hex_str(val);
        }
        // unknown keys: silently ignore — forward compat
    }

    fclose(f);
    return true;
}

// parse "#rrggbb" string
static rgb rgb_hex_str(const char* s)
{
    if (s[0] == '#') {
        s++;
    }
    uint32_t hex = (uint32_t)strtoul(s, NULL, 16);
    return rgb_hex(hex);
}




static border_style parse_border_style(const char* s)
{
    if (strcmp(s, "rounded") == 0) {
        return BORDER_ROUNDED;
    }
    if (strcmp(s, "bold") == 0) {
        return BORDER_BOLD;
    }
    if (strcmp(s, "double") == 0) {
        return BORDER_DOUBLE;
    }
    return BORDER_SHARP; // default
}

static cursor_style parse_cursor_style(const char* s)
{
    if (strcmp(s, "bar") == 0) {
        return CURSOR_BAR;
    }
    if (strcmp(s, "underline") == 0) {
        return CURSOR_UNDERLINE;
    }
    if (strcmp(s, "none") == 0) {
        return CURSOR_NONE;
    }
        // default
    return CURSOR_BLOCK;
}


