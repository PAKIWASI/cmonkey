#include "config.h"

#include <stdlib.h>
#include <string.h>



static rgb          rgb_hex_str(const char* s);
static border_style parse_border_style(const char* s);
static cursor_style parse_cursor_style(const char* s);

// Parse both quoted strings  conf[key]="value"
// and bare numbers           conf[key]=42
// Returns true if key+val were populated.
static bool parse_kv(const char* prefix, const char* line, char* key, char* val)
{
    // try quoted first
    char fmt_q[32];
    snprintf(fmt_q, sizeof(fmt_q), "%s[%%63[^]]]=\\\"%%63[^\\\"]\\\"", prefix);
    if (sscanf(line, fmt_q, key, val) == 2) {
        return true;
    }

    // try bare number
    char fmt_n[32];
    snprintf(fmt_n, sizeof(fmt_n), "%s[%%63[^]]]=%%63s", prefix);
    if (sscanf(line, fmt_n, key, val) == 2) {
        return true;
    }

    return false;
}


bool cmonkey_import_conf(cmonkey_conf* conf, const char* confpath)
{
    FILE* f = fopen(confpath, "r");
    if (!f) {
        return false;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char* p = line;
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == '#' || *p == '\n' || *p == '\0') {
            continue;
        }

        char key[64], val[64];
        if (!parse_kv("conf", p, key, val)) {
            continue;
        }

        if (strcmp(key, "trail_len") == 0) {
            conf->cursor_trail_len = (u8)atoi(val);
        } else if (strcmp(key, "trail_decay_ms") == 0) {
            conf->cursor_trail_decay_ms = (float)atof(val);
        }
        // unknown keys: silently ignore
    }

    fclose(f);
    LOG("config loaded from %s", confpath);
    return true;
}


bool cmonkey_import_theme(cmonkey_theme* t, const char* themepath)
{
    // derive name from filename (basename, no extension)
    const char* slash   = strrchr(themepath, '/');
    const char* base    = slash ? slash + 1 : themepath;
    const char* dot     = strrchr(base, '.');
    u32         namelen = dot ? (u32)(dot - base) : (u32)strlen(base);
    if (namelen >= sizeof(t->name)) {
        namelen = (u32)sizeof(t->name) - 1;
    }
    strncpy(t->name, base, namelen);
    t->name[namelen] = '\0';

    FILE* f = fopen(themepath, "r");
    if (!f) {
        return false;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) 
    {
        char* p = line;
        while (*p == ' ' || *p == '\t') {
            p++;
        }
        if (*p == '#' || *p == '\n' || *p == '\0') {
            continue;
        }

        char key[64], val[64];

        // theme file may also contain conf[] keys (e.g. border_style, cursor_style)
        bool is_theme = parse_kv("theme", p, key, val);
        bool is_conf  = !is_theme && parse_kv("conf", p, key, val);

        if (!is_theme && !is_conf) {
            continue;
        }

        if (is_theme) {
            if (strcmp(key, "bg") == 0) {
                if (strlen(val) > 0) {
                    t->base.bg     = rgb_hex_str(val);
                    t->base.has_bg = true;
                }
            } else if (strcmp(key, "fg") == 0) {
                t->base.fg      = rgb_hex_str(val);
                t->main_text.fg = rgb_hex_str(val); // main_text defaults to fg
            } else if (strcmp(key, "main_text") == 0) {
                t->main_text.fg = rgb_hex_str(val); // explicit override
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
        }

        if (is_conf || is_theme) {
            // these keys are accepted under either prefix for convenience
            if (strcmp(key, "border_style") == 0) {
                t->border_style = (u8)parse_border_style(val);
            } else if (strcmp(key, "cursor_style") == 0) {
                t->cursor_style = (u8)parse_cursor_style(val);
            }
        }
    }

    fclose(f);
    LOG("theme loaded from %s", themepath);
    return true;
}

static rgb rgb_hex_str(const char* s)
{
    if (s[0] == '#') {
        s++;
    }
    u32 hex = (u32)strtoul(s, NULL, 16);
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
    return BORDER_SHARP;
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
    return CURSOR_BLOCK;
}


