#include "common_single.h"
#include "draw.h"
#include "config.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>


/*
# cmonkey theme — dracula
main_fg     = "#f8f8f2"
  │              │
  key          value
  │              │
  ▼              ▼
theme_set_color ──► hex_to_escape(true, ...)
                        │
                        ▼
                    t->main_fg = "\033[38;2;248;248;242m"

text_fg     =
  │
  key, value_len == 0
  │
  ▼
continue ──► t->text_fg stays "" (empty, inherited)

*/

static void theme_set_color(cmonkey_theme *t, const char *key, const char *hex);
static void hex_to_escape(const char* hex, bool is_fg,
                          char esc[static COLOR_ESC_MAX]);

/*
 * Strip a trailing comment from `line`, but only if the '#' is NOT inside
 * a quoted value.  Rules:
 *   - scan left-to-right, track whether we are inside double-quotes
 *   - a '#' outside quotes → comment start → truncate there
 *   - quotes are not nested and are not escaped (matches the .theme syntax)
 */
static void strip_comment(char* line)
{
    bool in_quotes = false;
    for (char* p = line; *p; p++) {
        if (*p == '"')  { in_quotes = !in_quotes; }
        if (*p == '#' && !in_quotes) { *p = '\0'; return; }
    }
}

/* shared line parser
 * Fills `key` and `value` (both caller-supplied, size >= 64).
 * Returns true if a valid "key = value" pair was found.
 * value may be empty (caller decides what to do with that).
 */
static bool parse_kv(char* line, int line_num, const char* ctx,
                     char key[64], char value[64])
{
    strip_comment(line);

    // Trim leading whitespace
    char* p = line;
    while (*p && isspace((u8)*p)) { p++; }

    // Skip blank lines
    if (*p == '\0' || *p == '\n') { return false; }

    // Find '='
    char* eq = strchr(p, '=');
    if (!eq) {
        WARN("%s:%d: missing '=', skipping line\n", ctx, line_num);
        return false;
    }

    // Key: everything before '=', trailing-whitespace trimmed
    u64 key_len = (u64)(eq - p);
    while (key_len > 0 && isspace((u8)p[key_len - 1])) { key_len--; }
    memcpy(key, p, key_len);
    key[key_len] = '\0';        // add nul term

    // Value: everything after '=', whitespace trimmed on both ends
    char* val_start = eq + 1;
    while (*val_start && isspace((u8)*val_start)) { val_start++; }
    char* val_end = val_start + strlen(val_start);
    while (val_end > val_start && isspace((u8)*(val_end - 1))) { val_end--; }

    // Strip surrounding double-quotes if present
    if (val_end - val_start >= 2 &&
        *val_start == '"' && *(val_end - 1) == '"') {
        val_start++;
        val_end--;
    }

    u64 val_len = (u64)(val_end - val_start);
    memcpy(value, val_start, val_len);
    value[val_len] = '\0';      // add null term

    return true;
}


// theme

void theme_load(cmonkey_theme* t, const char* filepath)
{
    FILE* fp = fopen(filepath, "r");
    if (!fp) {
        WARN("Cannot open theme '%s', using defaults\n", filepath);
        return;
    }

    char line[256];
    int  line_num = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_num++;

        char key[64]   = {0};
        char value[64] = {0};

        if (!parse_kv(line, line_num, "theme", key, value)) { continue; }

        // TODO: if text_fg/bg is empty, set main_bg/fg for it
        // Empty value → "keep at default" → skip
        if (value[0] == '\0') { continue; }

        // printf("%s, %s\n", key, value);
        theme_set_color(t, key, value);
    }

    fclose(fp);

    // Build the global reset string
    snprintf(t->reset, sizeof(t->reset), "\033[0m%s%s",
             t->main_fg, t->main_bg);
}


static void theme_set_color(cmonkey_theme* t, const char* key, const char* hex)
{
    struct {
        const char* name;
        char*       color;
        bool        is_fg;
    } map[] = {
        { "main_fg",   t->main_fg,   true  },
        { "main_bg",   t->main_bg,   false },
        { "border",    t->border,    true  },
        { "cursor",    t->cursor,    true  },
        { "text_fg",   t->text_fg,   true  },
        { "text_bg",   t->text_bg,   false },
        { "text_dim",  t->text_dim,  true  },
        { "correct",   t->correct,   true  },
        { "incorrect", t->incorrect, true  },
    };

    for (u32 i = 0; i < sizeof(map)/sizeof(map[0]); i++) {
        if (strcmp(key, map[i].name) == 0) {
            hex_to_escape(hex, map[i].is_fg, map[i].color);
            return;
        }
    }

    WARN("theme: unknown key '%s'\n", key);
}

// convert hex colour "#aabbcc" → ANSI escape "\033[38;2;r;g;bm"
static void hex_to_escape(const char* hex, bool is_fg,
                          char esc[static COLOR_ESC_MAX])
{
    if (!hex || hex[0] != '#') {
        WARN("invalid hex code: %s", hex ? hex : "(null)");
        return;
    }

    size_t len = strlen(hex);
    if (len != 7) {
        WARN("hex must be #rrggbb format, got length %zu", len);
        return;
    }

    u32 r, g, b;
    if (sscanf(hex + 1, "%2x%2x%2x", &r, &g, &b) != 3) {
        WARN("invalid hex digits in '%s'", hex);
        return;
    }

    snprintf(esc, COLOR_ESC_MAX,
             ESC "%s;2;%u;%u;%um",
             (is_fg ? "38" : "48"), r, g, b);
}


// conf

void config_load(cmonkey_conf* c, const char* filepath)
{
    FILE* fp = fopen(filepath, "r");
    if (!fp) {
        WARN("Cannot open config '%s', using defaults\n", filepath);
        return;
    }

    char line[256];
    int  line_num = 0;

    while (fgets(line, sizeof(line), fp)) {
        line_num++;

        char key[64]   = {0};
        char value[64] = {0};

        if (!parse_kv(line, line_num, "conf", key, value)) { continue; }
        if (value[0] == '\0') { continue; }

        // printf("%s, %s\n", key, value);
        if (strcmp(key, "border_style") == 0) {
            if      (strcmp(value, "sharp")   == 0) { c->border_style = BORDER_SHARP;  }
            else if (strcmp(value, "rounded") == 0) { c->border_style = BORDER_ROUND;  }
            else if (strcmp(value, "bold")    == 0) { c->border_style = BORDER_BOLD;   }
            else if (strcmp(value, "double")  == 0) { c->border_style = BORDER_DOUBLE; }
            else { WARN("conf:%d: unknown border_style '%s'\n", line_num, value); }
        }
        else if (strcmp(key, "cursor_style") == 0) {
            if      (strcmp(value, "bar")       == 0) { c->cursor_style = CURSOR_BAR;       }
            else if (strcmp(value, "block")     == 0) { c->cursor_style = CURSOR_BLOCK;      }
            else if (strcmp(value, "underline") == 0) { c->cursor_style = CURSOR_UNDERLINE;  }
            else { WARN("conf:%d: unknown cursor_style '%s'\n", line_num, value); }
        }
        // trail_len / trail_decay_ms are marked "for later" in the struct
        // silently ignore them for now
    }

    fclose(fp);
}


