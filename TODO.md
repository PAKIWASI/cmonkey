Great question — btop's approach is exactly right. Here's a solid architecture for a C terminal rendering utility built around string building with ANSI escape codes.

## Core concepts

**ANSI escape codes** follow the pattern `\033[<params>m` for colors/styles and `\033[<row>;<col>H` for cursor positioning. You build strings containing these codes, then write them to stdout in one shot. This avoids flickering and keeps rendering fast.

**The btop philosophy**: maintain a "frame buffer" — build the entire frame as strings, then flush everything at once with a single `write()`.

Here's the architecture I'd recommend:

## Layer 0 — theme

```c
// theme.h
typedef struct {
    uint8_t r, g, b;
} rgb_t;

typedef struct {
    rgb_t fg, bg;        // defaults
    rgb_t accent;        // highlights, cursor
    rgb_t correct;       // correct keystrokes
    rgb_t wrong;         // wrong keystrokes
    rgb_t dim;           // ghost/upcoming text

    // border style: 0=sharp, 1=rounded, 2=bold
    int border_style;
} theme_t;

/* border character sets indexed by style enum */
typedef enum { BORDER_SHARP=0, BORDER_ROUNDED=1, BORDER_BOLD=2, BORDER_DOUBLE=3 } border_style_t;

static const char *BORDER_CHARS[4][6] = {
    /* tl      tr      bl      br      v    h   */
    { "┌",   "┐",   "└",   "┘",   "│", "─" },   /* sharp   */
    { "╭",   "╮",   "╰",   "╯",   "│", "─" },   /* rounded */
    { "┏",   "┓",   "┗",   "┛",   "┃", "━" },   /* bold    */
    { "╔",   "╗",   "╚",   "╝",   "║", "═" },   /* double  */
};
```

## Layer 1 — frame buffer

```c
// term_buf.h
typedef struct {
    char  *data;
    size_t len;
    size_t cap;
} term_buf_t;

// TODO: double buffering ??

void   tb_init(term_buf_t *b, size_t initial_cap);
void   tb_free(term_buf_t *b);
void   tb_append(term_buf_t *b, const char *s);
void   tb_appendn(term_buf_t *b, const char *s, size_t n);
void   tb_appendf(term_buf_t *b, const char *fmt, ...);  // printf-style
void   tb_flush(term_buf_t *b);   // write() to STDOUT_FILENO, reset len=0
```

```c
// term_buf.c
void tb_appendf(term_buf_t *b, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    // vsnprintf into b->data + b->len, grow if needed
    int needed = vsnprintf(b->data + b->len, b->cap - b->len, fmt, ap);
    if ((size_t)needed >= b->cap - b->len) {
        b->cap = b->len + needed + 256;
        b->data = realloc(b->data, b->cap);
        va_start(ap, fmt);
        vsnprintf(b->data + b->len, b->cap - b->len, fmt, ap);
    }
    b->len += needed;
    va_end(ap);
}

void tb_flush(term_buf_t *b) {
    write(STDOUT_FILENO, b->data, b->len);
    b->len = 0;
}
```

## Layer 2 — draw primitives

```c
// draw.h  — all take term_buf_t* and write ANSI into it

// cursor
void draw_move(term_buf_t *b, int row, int col);
    // → tb_appendf(b, "\033[%d;%dH", row, col)

// colors (true color)
void draw_fg(term_buf_t *b, rgb_t c);
    // → tb_appendf(b, "\033[38;2;%d;%d;%dm", c.r, c.g, c.b)

void draw_bg(term_buf_t *b, rgb_t c);
    // → tb_appendf(b, "\033[48;2;%d;%d;%dm", c.r, c.g, c.b)

void draw_reset(term_buf_t *b);
    // → tb_append(b, "\033[0m")

void draw_bold(term_buf_t *b);
    // → tb_append(b, "\033[1m")

void draw_dim(term_buf_t *b);
    // → tb_append(b, "\033[2m")

// screen
void draw_clear(term_buf_t *b);
    // → tb_append(b, "\033[2J\033[H")

void draw_hide_cursor(term_buf_t *b);
    // → tb_append(b, "\033[?25l")

void draw_show_cursor(term_buf_t *b);
    // → tb_append(b, "\033[?25h")
```

## Layer 3 — widgets

```c
// Rounded border box
void widget_border(term_buf_t *b, const theme_t *t,
                   int row, int col, int w, int h,
                   const char *title)
{
    const char **ch = BORDERS[t->border_style];
    // top edge
    draw_move(b, row, col);
    draw_fg(b, t->accent);
    tb_append(b, ch[0]);                   // ╭
    for (int i = 0; i < w - 2; i++)
        tb_append(b, ch[5]);               // ─
    tb_append(b, ch[1]);                   // ╮

    if (title) {
        draw_move(b, row, col + 2);
        draw_fg(b, t->fg);
        tb_appendf(b, " %s ", title);
        draw_fg(b, t->accent);
    }

    // sides
    for (int r = 1; r < h - 1; r++) {
        draw_move(b, row + r, col);
        tb_append(b, ch[4]);               // │
        draw_move(b, row + r, col + w - 1);
        tb_append(b, ch[4]);
    }

    // bottom edge
    draw_move(b, row + h - 1, col);
    tb_append(b, ch[2]);                   // ╰
    for (int i = 0; i < w - 2; i++)
        tb_append(b, ch[5]);
    tb_append(b, ch[3]);                   // ╯

    draw_reset(b);
}

// Colored text word (for the typing display)
void widget_word_char(term_buf_t *b, const theme_t *t,
                      const char *utf8_char, int state)
{
    // state: 0=upcoming, 1=correct, 2=wrong, 3=cursor
    rgb_t color = (state == 1) ? t->correct :
                  (state == 2) ? t->wrong   :
                  (state == 3) ? t->accent  : t->dim;
    if (state == 3) draw_bg(b, t->accent), draw_fg(b, t->bg);
    else            draw_fg(b, color);
    tb_append(b, utf8_char);
    draw_reset(b);
}
```

## UTF-8 and multi-byte characters

The key insight for multi-language support: **ANSI cursor positions are in terminal columns, not bytes**. A CJK character is 2 columns wide, most Latin/Arabic/Hebrew are 1. You need `wcwidth()`:

```c
#include <wchar.h>
#include <locale.h>

// At startup:
setlocale(LC_ALL, "");

// When measuring a grapheme cluster's display width:
// convert UTF-8 → wchar_t, then:
int cols = wcwidth(wc);   // -1=control, 0=combining, 1=normal, 2=wide (CJK)
```

For a typing TUI, store your word list as UTF-8 strings, iterate over them as grapheme clusters (use a small lib like `utf8.h` by sheredom, or hand-roll a UTF-8 decoder), and track **column position** separately from **byte offset**.

## Terminal setup / teardown

```c
#include <termios.h>

static struct termios orig_termios;

void term_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);  // no echo, char-by-char input
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void term_restore(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void term_get_size(int *rows, int *cols) {
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    *rows = ws.ws_row;
    *cols = ws.ws_col;
}
```

Register `term_restore` with `atexit()` and a `SIGTERM`/`SIGINT` handler so you never leave the terminal broken.

## The game loop

```c
void run(void) {
    term_buf_t buf;
    tb_init(&buf, 65536);

    while (!quit) {
        int rows, cols;
        term_get_size(&rows, &cols);

        // build entire frame
        draw_clear(&buf);
        draw_hide_cursor(&buf);

        widget_border(&buf, &theme, 2, 4, cols - 8, rows - 4, "typefast");
        // ... render words, wpm counter, progress bar ...

        tb_flush(&buf);          // single write() call

        // read one keypress (or use poll() for non-blocking)
        handle_input();
    }
}
```

## Theme loading from file

For real customizability, load themes from a simple config (TOML or your own format):

```
[colors]
bg      = "#1e1e2e"
fg      = "#cdd6f4"
accent  = "#cba6f7"
correct = "#a6e3a1"
wrong   = "#f38ba8"
dim     = "#585b70"

[style]
border = rounded   # sharp | rounded | bold
```

Parse hex colors with `strtol(hex+1, NULL, 16)` and extract R/G/B bytes via bit shifts.

---

The whole thing stays dependency-free (libc only, plus an optional `utf8.h`). For wide character width you can also vendor `mk_wcwidth.c` (Markus Kuhn's implementation) which handles all Unicode edge cases better than some platform `wcwidth()` implementations.


These are all trivially thin wrappers — here's the complete implementation plus the color/theme integration:

## draw.c — full implementation

```c
#include "draw.h"
#include "term_buf.h"

/* ── cursor ─────────────────────────────────────────────────── */

void draw_move(term_buf_t *b, int row, int col) {
    tb_appendf(b, "\033[%d;%dH", row, col);
}

void draw_move_col(term_buf_t *b, int col) {
    tb_appendf(b, "\033[%dG", col);          // move within current row
}

// TODO: diff cursor shapes?

/* ── SGR attributes ──────────────────────────────────────────── */

void draw_reset(term_buf_t *b)   { tb_append(b, "\033[0m");  }
void draw_bold(term_buf_t *b)    { tb_append(b, "\033[1m");  }
void draw_dim(term_buf_t *b)     { tb_append(b, "\033[2m");  }
void draw_italic(term_buf_t *b)  { tb_append(b, "\033[3m");  }
void draw_underline(term_buf_t *b){ tb_append(b, "\033[4m"); }
void draw_blink(term_buf_t *b)   { tb_append(b, "\033[5m");  }
void draw_reverse(term_buf_t *b) { tb_append(b, "\033[7m");  }
void draw_strike(term_buf_t *b)  { tb_append(b, "\033[9m");  }

/* ── true color (24-bit) ─────────────────────────────────────── */

void draw_fg(term_buf_t *b, rgb_t c) {
    tb_appendf(b, "\033[38;2;%d;%d;%dm", c.r, c.g, c.b);
}

void draw_bg(term_buf_t *b, rgb_t c) {
    tb_appendf(b, "\033[48;2;%d;%d;%dm", c.r, c.g, c.b);
}

/* combined: set both fg and bg in one call (saves a tb_appendf) */
void draw_color(term_buf_t *b, rgb_t fg, rgb_t bg) {
    tb_appendf(b, "\033[38;2;%d;%d;%d;48;2;%d;%d;%dm",
               fg.r, fg.g, fg.b,
               bg.r, bg.g, bg.b);
}

/* ── screen ──────────────────────────────────────────────────── */

void draw_clear(term_buf_t *b) {
    tb_append(b, "\033[2J\033[H");           // erase screen + home cursor
}

void draw_clear_line(term_buf_t *b) {
    tb_append(b, "\033[2K");                 // erase entire current line
}

void draw_clear_to_eol(term_buf_t *b) {
    tb_append(b, "\033[K");                  // erase from cursor to end of line
}

void draw_hide_cursor(term_buf_t *b) { tb_append(b, "\033[?25l"); }
void draw_show_cursor(term_buf_t *b) { tb_append(b, "\033[?25h"); }

/* save/restore cursor position (useful for overlays) */
void draw_save_cursor(term_buf_t *b)    { tb_append(b, "\033[s"); }
void draw_restore_cursor(term_buf_t *b) { tb_append(b, "\033[u"); }

/* enter/exit alternate screen buffer — use this so the terminal
   restores its scrollback when your app exits                    */
void draw_alt_screen_enter(term_buf_t *b) { tb_append(b, "\033[?1049h"); }
void draw_alt_screen_exit(term_buf_t *b)  { tb_append(b, "\033[?1049l"); }
```

## Borders with theming

The border function takes a `theme_slot_t` so callers choose which color to draw from — you're not hardcoding accent everywhere:

```c
/* theme.h */
typedef struct { uint8_t r, g, b; } rgb_t;

/* named color slots — add as many as you need */
typedef struct {
    rgb_t bg;
    rgb_t fg;
    rgb_t dim;
    rgb_t accent;
    rgb_t correct;
    rgb_t wrong;
    rgb_t border_normal;
    rgb_t border_focused;
    rgb_t border_title;
    rgb_t status_bar_bg;
    rgb_t status_bar_fg;
} theme_t;

/* border character sets indexed by style enum */
typedef enum { BORDER_SHARP=0, BORDER_ROUNDED=1, BORDER_BOLD=2, BORDER_DOUBLE=3 } border_style_t;

static const char *BORDER_CHARS[4][6] = {
    /* tl      tr      bl      br      v    h   */
    { "┌",   "┐",   "└",   "┘",   "│", "─" },   /* sharp   */
    { "╭",   "╮",   "╰",   "╯",   "│", "─" },   /* rounded */
    { "┏",   "┓",   "┗",   "┛",   "┃", "━" },   /* bold    */
    { "╔",   "╗",   "╚",   "╝",   "║", "═" },   /* double  */
};
```

```c
/* draw.c — themed border */

typedef struct {
    rgb_t         border_color;
    rgb_t         title_color;
    rgb_t         title_bg;     /* often same as global bg */
    border_style_t style;
    int           title_bold;
} border_opts_t;

void draw_border(term_buf_t *b, const border_opts_t *o,
                 int row, int col, int w, int h,
                 const char *title)
{
    const char **ch = BORDER_CHARS[o->style];

    draw_fg(b, o->border_color);

    /* ── top edge ── */
    draw_move(b, row, col);
    tb_append(b, ch[0]);                         /* ╭ */
    for (int i = 0; i < w - 2; i++)
        tb_append(b, ch[5]);                     /* ─ */
    tb_append(b, ch[1]);                         /* ╮ */

    /* ── title ── injected into top edge */
    if (title) {
        draw_move(b, row, col + 2);
        draw_fg(b, o->title_color);
        draw_bg(b, o->title_bg);
        if (o->title_bold) draw_bold(b);
        tb_appendf(b, " %s ", title);
        draw_reset(b);
        draw_fg(b, o->border_color);             /* restore for sides */
    }

    /* ── sides ── */
    for (int r = 1; r < h - 1; r++) {
        draw_move(b, row + r, col);
        tb_append(b, ch[4]);                     /* │ */
        draw_move(b, row + r, col + w - 1);
        tb_append(b, ch[4]);
    }

    /* ── bottom edge ── */
    draw_move(b, row + h - 1, col);
    tb_append(b, ch[2]);                         /* ╰ */
    for (int i = 0; i < w - 2; i++)
        tb_append(b, ch[5]);
    tb_append(b, ch[3]);                         /* ╯ */

    draw_reset(b);
}
```

Calling it — normal vs focused box:

```c
border_opts_t normal = {
    .border_color = theme.border_normal,
    .title_color  = theme.fg,
    .title_bg     = theme.bg,
    .style        = BORDER_ROUNDED,
    .title_bold   = 0,
};

border_opts_t focused = {
    .border_color = theme.border_focused,  /* bright accent */
    .title_color  = theme.accent,
    .title_bg     = theme.bg,
    .style        = BORDER_ROUNDED,
    .title_bold   = 1,
};

draw_border(&buf, is_focused ? &focused : &normal, row, col, w, h, "words");
```

## Colored text — the typing display

```c
typedef enum {
    CHAR_UPCOMING = 0,
    CHAR_CORRECT  = 1,
    CHAR_WRONG    = 2,
    CHAR_CURSOR   = 3,
    CHAR_EXTRA    = 4,   /* typed beyond word length */
} char_state_t;

void draw_char(term_buf_t *b, const theme_t *t,
               const char *utf8_grapheme, char_state_t state)
{
    switch (state) {
    case CHAR_UPCOMING:
        draw_fg(b, t->dim);
        break;
    case CHAR_CORRECT:
        draw_fg(b, t->correct);
        break;
    case CHAR_WRONG:
        draw_fg(b, t->wrong);
        draw_underline(b);               /* extra cue beyond color */
        break;
    case CHAR_CURSOR:
        draw_bg(b, t->accent);           /* block cursor effect */
        draw_fg(b, t->bg);
        break;
    case CHAR_EXTRA:
        draw_fg(b, t->wrong);
        draw_dim(b);
        break;
    }

    tb_append(b, utf8_grapheme);
    draw_reset(b);
}
```

## rgb_t helpers

You'll use these constantly — saves typing inline everywhere:

```c
/* construct from hex literal: rgb_hex(0xcba6f7) */
static inline rgb_t rgb_hex(uint32_t hex) {
    return (rgb_t){ (hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF };
}

/* lerp between two colors — good for animations, progress bars */
static inline rgb_t rgb_lerp(rgb_t a, rgb_t b, float t) {
    return (rgb_t){
        (uint8_t)(a.r + (b.r - a.r) * t),
        (uint8_t)(a.g + (b.g - a.g) * t),
        (uint8_t)(a.b + (b.b - a.b) * t),
    };
}

/* dim a color toward black — for inactive/unfocused state */
static inline rgb_t rgb_dim(rgb_t c, float factor) {
    return (rgb_t){ c.r * factor, c.g * factor, c.b * factor };
}
```

## Defining a theme

```c
/* themes/catppuccin_mocha.c */
const theme_t THEME_CATPPUCCIN_MOCHA = {
    .bg            = { 30,  30,  46  },   /* #1e1e2e base    */
    .fg            = { 205, 214, 244 },   /* #cdd6f4 text    */
    .dim           = { 88,  91,  112 },   /* #585b70 overlay */
    .accent        = { 203, 166, 247 },   /* #cba6f7 mauve   */
    .correct       = { 166, 227, 161 },   /* #a6e3a1 green   */
    .wrong         = { 243, 139, 168 },   /* #f38ba8 red     */
    .border_normal = { 88,  91,  112 },   /* overlay1        */
    .border_focused= { 203, 166, 247 },   /* mauve           */
    .border_title  = { 205, 214, 244 },   /* text            */
    .status_bar_bg = { 24,  24,  37  },   /* mantle          */
    .status_bar_fg = { 166, 173, 200 },   /* subtext1        */
};

const theme_t THEME_GRUVBOX_DARK = {
    .bg            = { 40,  40,  40  },
    .fg            = { 235, 219, 178 },
    .dim           = { 102, 92,  84  },
    .accent        = { 250, 189, 47  },
    .correct       = { 184, 187, 38  },
    .wrong         = { 251, 73,  52  },
    .border_normal = { 80,  73,  69  },
    .border_focused= { 250, 189, 47  },
    .border_title  = { 235, 219, 178 },
    .status_bar_bg = { 29,  32,  33  },
    .status_bar_fg = { 168, 153, 132 },
};
```

Switch themes at runtime by just swapping the `theme_t *` pointer — every widget reads from it on each frame so it takes effect immediately.

## Putting it together — one frame

```c
void render_frame(term_buf_t *buf, const state_t *s, const theme_t *t) {
    int rows, cols;
    term_get_size(&rows, &cols);

    draw_clear(buf);
    draw_bg(buf, t->bg);                 /* flood fill background */

    /* main typing box */
    border_opts_t bo = {
        .border_color = s->typing ? t->border_focused : t->border_normal,
        .title_color  = t->accent,
        .title_bg     = t->bg,
        .style        = BORDER_ROUNDED,
        .title_bold   = 1,
    };
    draw_border(buf, &bo, 3, 4, cols - 8, rows - 6, "typefast");

    /* words inside box */
    draw_move(buf, 5, 6);
    for each grapheme in s->current_word {
        draw_char(buf, t, grapheme, grapheme_state);
    }

    /* status bar */
    draw_move(buf, rows - 1, 1);
    draw_color(buf, t->status_bar_fg, t->status_bar_bg);
    tb_appendf(buf, "  wpm: %-3d  acc: %d%%  time: %ds  ",
               s->wpm, s->accuracy, s->seconds_left);
    draw_reset(buf);

    tb_flush(buf);
}
```

One thing worth noting: always call `draw_alt_screen_enter` at startup and `draw_alt_screen_exit` on exit — this keeps the terminal's scrollback intact so your TUI doesn't trash the user's history.


## Multiple windows / panels

The key insight: a "window" is just a **clipping region** — a row/col origin plus width/height. You never draw to the whole screen; you draw into a window's bounds. Stack them in a z-ordered list and composite front-to-back.

```c
// window.h
typedef struct {
    int row, col;     // top-left (1-indexed)
    int w, h;
    int z;            // higher = on top
    int visible;
    int dirty;        // needs redraw this frame
    char *title;

    // each window owns a cell buffer (its "back buffer")
    cell_t *cells;    // w * h cells
} window_t;
```

## Cell-based double buffering

Instead of buffering raw ANSI strings, buffer **cells**. Each cell holds one grapheme + its colors + attributes. The differ compares old vs new and only emits ANSI for cells that changed.

```c
// cell.h
typedef struct {
    char    ch[5];      // UTF-8 grapheme (max 4 bytes + NUL)
    rgb_t   fg, bg;
    uint8_t attrs;      // ATTR_BOLD | ATTR_DIM | ATTR_UNDERLINE | ATTR_REVERSE
    uint8_t dirty;
} cell_t;

#define ATTR_BOLD      (1 << 0)
#define ATTR_DIM       (1 << 1)
#define ATTR_UNDERLINE (1 << 2)
#define ATTR_ITALIC    (1 << 3)
#define ATTR_REVERSE   (1 << 4)
```

The screen owns two full-screen cell grids — front and back:

```c
// screen.h
typedef struct {
    int     rows, cols;
    cell_t *front;     // what the terminal currently shows
    cell_t *back;      // what we're building this frame
    term_buf_t emit;   // ANSI output buffer
} screen_t;

void screen_init(screen_t *s, int rows, int cols) {
    s->rows  = rows;
    s->cols  = cols;
    size_t n = rows * cols;
    s->front = calloc(n, sizeof(cell_t));
    s->back  = calloc(n, sizeof(cell_t));
    tb_init(&s->emit, screen_buf_size(cols, rows));

    // poison front so first frame is a full repaint
    memset(s->front, 0xFF, n * sizeof(cell_t));
}

// call on SIGWINCH
void screen_resize(screen_t *s, int rows, int cols) {
    free(s->front); free(s->back);
    // TODO: also destroy term_buf
    screen_init(s, rows, cols);
}
```

## The differ — core of efficient redraw

```c
static inline int cell_eq(const cell_t *a, const cell_t *b) {
    return memcmp(a->ch, b->ch, 5) == 0
        && memcmp(&a->fg, &b->fg, sizeof(rgb_t)) == 0
        && memcmp(&a->bg, &b->bg, sizeof(rgb_t)) == 0
        && a->attrs == b->attrs;
}

void screen_flush(screen_t *s) {
    term_buf_t *b = &s->emit;
    b->len = 0;

    draw_hide_cursor(b);

    rgb_t   cur_fg    = {0,0,0};
    rgb_t   cur_bg    = {0,0,0};
    uint8_t cur_attrs = 0xFF;          // force first emit
    int     cur_row   = -1;
    int     cur_col   = -1;

    for (int r = 0; r < s->rows; r++) {
        int col_run = -1;              // tracks if we can skip draw_move

        for (int c = 0; c < s->cols; c++) {
            cell_t *front = &s->front[r * s->cols + c];
            cell_t *back  = &s->back [r * s->cols + c];

            if (cell_eq(front, back)) continue;   // ← the key skip

            // only emit move if not continuing a run
            if (cur_row != r || cur_col != c) {
                draw_move(b, r + 1, c + 1);
                cur_row = r;
            }
            cur_col = c + 1;

            // only re-emit color/attrs if changed
            if (!rgb_eq(back->fg, cur_fg) || !rgb_eq(back->bg, cur_bg)
                || back->attrs != cur_attrs)
            {
                emit_attrs(b, back->fg, back->bg, back->attrs,
                           cur_fg, cur_bg, cur_attrs);
                cur_fg    = back->fg;
                cur_bg    = back->bg;
                cur_attrs = back->attrs;
            }

            tb_append(b, back->ch[0] ? back->ch : " ");
            *front = *back;            // sync front
        }
    }

    draw_reset(b);
    draw_show_cursor(b);
    tb_flush(b);        // single write() call and set buff len to 0
}
```

`emit_attrs` only sends the SGR codes that actually changed — bold on/off, individual color codes — not a blanket reset every cell:

```c
static void emit_attrs(term_buf_t *b,
                        rgb_t nfg, rgb_t nbg, uint8_t nattr,
                        rgb_t ofg, rgb_t obg, uint8_t oattr)
{
    // if attrs cleared (new has fewer bits set), must reset first
    if (oattr & ~nattr) {
        tb_append(b, "\033[0m");
        oattr = 0;
        // colors now gone too, force re-emit
        ofg = (rgb_t){0xFF,0xFF,0xFF};
        obg = (rgb_t){0xFF,0xFF,0xFF};
    }

    if ((nattr & ATTR_BOLD)      && !(oattr & ATTR_BOLD))      tb_append(b, "\033[1m");
    if ((nattr & ATTR_DIM)       && !(oattr & ATTR_DIM))       tb_append(b, "\033[2m");
    if ((nattr & ATTR_ITALIC)    && !(oattr & ATTR_ITALIC))    tb_append(b, "\033[3m");
    if ((nattr & ATTR_UNDERLINE) && !(oattr & ATTR_UNDERLINE)) tb_append(b, "\033[4m");
    if ((nattr & ATTR_REVERSE)   && !(oattr & ATTR_REVERSE))   tb_append(b, "\033[7m");

    if (!rgb_eq(nfg, ofg)) draw_fg(b, nfg);
    if (!rgb_eq(nbg, obg)) draw_bg(b, nbg);
}
```

## Writing to a window (compositing)

Widgets write cells into a window's local buffer. The window compositor then blits all windows onto the screen's back buffer, respecting z-order:

```c
// write a cell into a window's local buffer (bounds-checked)
void win_put(window_t *win, int row, int col, cell_t cell) {
    if (row < 0 || row >= win->h) return;
    if (col < 0 || col >= win->w) return;
    win->cells[row * win->w + col] = cell;
}

// convenience: write a string with given colors
void win_print(window_t *win, int row, int col,
               const char *utf8, rgb_t fg, rgb_t bg, uint8_t attrs)
{
    // iterate grapheme clusters, call win_put for each
    // (track column offset using wcwidth for wide chars)
}

// blit all windows onto screen back buffer (call once per frame)
void screen_composite(screen_t *s, window_t **wins, int nwins) {
    // clear back buffer to default bg first
    cell_t blank = { .ch=" ", .fg=theme.fg, .bg=theme.bg };
    for (int i = 0; i < s->rows * s->cols; i++)
        s->back[i] = blank;

    // paint windows low-z to high-z (painter's algorithm)
    // sort wins by z first, or keep pre-sorted
    for (int i = 0; i < nwins; i++) {
        window_t *w = wins[i];
        if (!w->visible) continue;
        for (int r = 0; r < w->h; r++) {
            int sr = w->row + r;
            if (sr < 0 || sr >= s->rows) continue;
            for (int c = 0; c < w->w; c++) {
                int sc = w->col + c;
                if (sc < 0 || sc >= s->cols) continue;
                s->back[sr * s->cols + sc] = w->cells[r * w->w + c];
            }
        }
    }
}
```

## Popups

A popup is just a high-z window created on demand and destroyed when dismissed. The windows underneath don't need to re-render — they're already composited:

```c
window_t *popup_open(int w, int h, const char *title) {
    int rows, cols;
    term_get_size(&rows, &cols);

    window_t *p = calloc(1, sizeof(window_t));
    p->row   = (rows - h) / 2;     // centered
    p->col   = (cols - w) / 2;
    p->w     = w;
    p->h     = h;
    p->z     = 100;                 // always on top
    p->visible = 1;
    p->cells = calloc(w * h, sizeof(cell_t));
    p->title = strdup(title);

    win_draw_border(p, &theme);     // fills border cells into p->cells
    return p;
}

void popup_close(window_t *p) {
    free(p->cells);
    free(p->title);
    free(p);
    // remove from wins[] list
}
```

Usage:

```c
// ESC pressed during typing → show quit confirmation
window_t *dlg = popup_open(36, 7, "quit?");
win_print(dlg, 2, 3, "quit the session?", theme.fg, theme.bg, 0);
win_print(dlg, 4, 5, "[y] yes   [n] no", theme.accent, theme.bg, ATTR_BOLD);
// add dlg to wins[], next composite pass paints it on top
```

## Buffer size

Don't guess — calculate from terminal dimensions. Worst case per cell is about 40 bytes of ANSI:

```
\033[999;999H   — move:       10 bytes
\033[38;2;255;255;255m — fg:  19 bytes
\033[48;2;255;255;255m — bg:  19 bytes
\033[1;3;4m    — attrs:        9 bytes
UTF-8 char     — grapheme:    up to 4 bytes
                               ─────────
worst case per cell:          ~61 bytes, call it 64
```

```c
size_t screen_buf_size(int cols, int rows) {
    // 64 bytes worst-case per cell
    // + 1024 bytes overhead (cursor, clear, reset sequences)
    return (size_t)cols * rows * 64 + 1024;
}
```

In practice with the differ skipping unchanged cells, you'll use 5–10% of that on a typical frame. But allocate the maximum upfront so `tb_appendf` never reallocs during a frame — that would cause a stutter. One allocation at startup, sized for the terminal, done.

```c
// on SIGWINCH — resize handler
void handle_resize(screen_t *s) {
    int rows, cols;
    term_get_size(&rows, &cols);

    screen_resize(s, rows, cols);            // reallocs front/back cell grids
    tb_resize(&s->emit, screen_buf_size(cols, rows));  // reallocs ANSI buffer

    // mark all windows dirty so they repopulate their cell buffers
    // screen_composite will do a full repaint next frame
}
```

## The full frame loop

```c
void run(app_t *app) {
    screen_t screen;
    term_get_size(&screen.rows, &screen.cols);
    screen_init(&screen, screen.rows, screen.cols);
    draw_alt_screen_enter(&screen.emit); tb_flush(&screen.emit);

    while (!app->quit) {
        // 1. each visible window re-renders into its own cell buffer
        for each window: if (win->dirty) widget_render(win, app->theme);

        // 2. composite all windows → screen.back
        screen_composite(&screen, app->wins, app->nwins);

        // 3. differ emits only changed cells → single write()
        screen_flush(&screen);

        // 4. block on input (or poll with timeout for animations)
        handle_input(app);
    }

    draw_alt_screen_exit(&screen.emit); tb_flush(&screen.emit);
}
```

The critical property: `screen_flush` is the **only** place that touches the real terminal. Everything else writes into cell buffers. This means popups, overlays, and focus changes are free — you just change which windows are visible and let the compositor + differ figure out the minimum ANSI to emit.



For a typing TUI, "smooth cursor" means a few distinct things — let's separate them:

1. **Visual cursor shape/style** — already covered (DECSCUSR)
2. **Cursor position** — always snapped to a cell, no sub-cell movement possible in a terminal
3. **Smooth *feel*** — this is what actually matters, and it's about timing + animation within cells

The terminal cell grid is discrete — you cannot position a cursor at pixel coordinates. But you can fake smooth movement convincingly.

## What btop/modern TUIs actually do

They don't move the real terminal cursor smoothly. They animate a **fake cursor** — a highlighted cell that moves through the cell grid — while the real cursor stays hidden or parked somewhere safe.

```c
// hide the real cursor entirely at startup
draw_hide_cursor(&buf);   // \033[?25l

// your "cursor" is just a cell rendered with inverted/highlighted colors
// move it by changing which cell gets the cursor style — each frame
```

## Fake cursor with interpolation

The trick is maintaining a **visual position** as a float, separate from the **logical position** as a cell index. Each frame you lerp the visual toward the logical:

```c
typedef struct {
    float    vis_col;       // visual position (float, for smooth lerp)
    float    vis_row;
    int      log_col;       // logical position (actual cell)
    int      log_row;
    uint64_t last_move_ms;  // when logical last changed
} cursor_t;

void cursor_update(cursor_t *c, float dt_sec) {
    float speed = 18.0f;    // cells per second — tune this
    
    float dc = c->log_col - c->vis_col;
    float dr = c->log_row - c->vis_row;
    
    // exponential ease-out: fast start, slow finish
    c->vis_col += dc * speed * dt_sec;
    c->vis_row += dr * speed * dt_sec;
    
    // snap when close enough (avoids infinite crawl)
    if (fabsf(dc) < 0.01f) c->vis_col = c->log_col;
    if (fabsf(dr) < 0.01f) c->vis_row = c->log_row;
}
```

Each frame, render the cell at `(int)round(c->vis_col)` with cursor highlighting. The cursor visibly glides between cells rather than jumping.

## The render loop with delta time

Smooth animation requires knowing how much time passed since the last frame:

```c
#include <time.h>

uint64_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000ULL;
}

void run(app_t *app) {
    uint64_t prev = now_ms();

    while (!app->quit) {
        uint64_t now = now_ms();
        float dt = (now - prev) / 1000.0f;   // seconds since last frame
        prev = now;

        // clamp dt — if window was minimized or system slept,
        // dt could be huge and cursor would teleport
        if (dt > 0.1f) dt = 0.1f;

        cursor_update(&app->cursor, dt);
        render_frame(app, dt);

        // target ~60fps: sleep remaining time in the 16ms budget
        uint64_t elapsed = now_ms() - now;
        if (elapsed < 16) {
            struct timespec ts = { 0, (16 - elapsed) * 1000000L };
            nanosleep(&ts, NULL);
        }

        // non-blocking input check
        if (term_wait_input(0) > 0)
            handle_input(app);
    }
}
```

## Cursor blink

Blink is a separate timer, independent of movement:

```c
typedef struct {
    // ...existing fields...
    uint64_t  blink_timer_ms;
    int       blink_on;
    int       blink_period_ms;   // e.g. 530ms — matches most terminals
    int       blink_enabled;
} cursor_t;

void cursor_blink_update(cursor_t *c, uint64_t now_ms) {
    if (!c->blink_enabled) { c->blink_on = 1; return; }

    if (now_ms - c->blink_timer_ms >= c->blink_period_ms) {
        c->blink_on ^= 1;
        c->blink_timer_ms = now_ms;
    }
}

// reset blink phase on every keypress — cursor stays solid
// while typing, blinks only during idle. Same behavior as VS Code.
void cursor_on_keypress(cursor_t *c, uint64_t now_ms) {
    c->blink_on = 1;
    c->blink_timer_ms = now_ms;
}
```

## Rendering the fake cursor

```c
void render_cursor(window_t *win, const cursor_t *c,
                   const theme_t *t, uint64_t now_ms)
{
    int col = (int)roundf(c->vis_col);
    int row = (int)roundf(c->vis_row);

    // bounds check
    if (col < 0 || col >= win->w) return;
    if (row < 0 || row >= win->h) return;

    cell_t *cell = &win->cells[row * win->w + col];

    if (!c->blink_on) return;   // blink off — don't highlight

    // swap fg/bg for block cursor effect
    cell->bg    = t->accent;
    cell->fg    = t->bg;
    cell->attrs |= ATTR_BOLD;
}
```

## Easing options

Different feels suit different TUI personalities:

```c
// 1. exponential ease-out (default — feels snappy)
vis += (target - vis) * speed * dt;

// 2. spring physics (overshoots slightly — feels alive)
typedef struct { float pos, vel; } spring_t;

void spring_update(spring_t *s, float target, float dt) {
    float stiffness = 200.0f;
    float damping   = 20.0f;
    float force = (target - s->pos) * stiffness - s->vel * damping;
    s->vel += force * dt;
    s->pos += s->vel * dt;
}

// 3. lerp with fixed step (consistent speed regardless of distance)
float dist = target - vis;
float step = speed * dt;                 // max cells to move this frame
if (fabsf(dist) <= step)
    vis = target;
else
    vis += copysignf(step, dist);

// 4. instant snap (no animation — for accessibility/preference)
vis = target;
```

Spring feels best for a typing TUI — it has a slight organic bounce that makes the cursor feel weighted rather than mechanical. Tune `stiffness` up for snappier, down for floatier.

## Cursor trail effect

A subtle ghost of where the cursor just was, fading out over ~150ms:

```c
typedef struct {
    int      col, row;
    uint64_t born_ms;       // when this trail cell was created
    float    alpha;         // 1.0 → 0.0 over lifetime_ms
} trail_cell_t;

#define MAX_TRAIL 4

typedef struct {
    trail_cell_t cells[MAX_TRAIL];
    int          head;
} cursor_trail_t;

void trail_push(cursor_trail_t *tr, int col, int row, uint64_t now) {
    tr->cells[tr->head] = (trail_cell_t){ col, row, now, 1.0f };
    tr->head = (tr->head + 1) % MAX_TRAIL;
}

void trail_render(cursor_trail_t *tr, window_t *win,
                  const theme_t *t, uint64_t now)
{
    uint64_t lifetime = 150;   // ms
    for (int i = 0; i < MAX_TRAIL; i++) {
        trail_cell_t *tc = &tr->cells[i];
        if (tc->col < 0) continue;

        uint64_t age = now - tc->born_ms;
        if (age >= lifetime) { tc->col = -1; continue; }

        float alpha = 1.0f - (float)age / lifetime;

        // blend accent color toward bg using alpha
        cell_t *cell = &win->cells[tc->row * win->w + tc->col];
        cell->bg = rgb_lerp(t->bg, t->accent, alpha * 0.35f);
    }
}
```

Trigger `trail_push` whenever the logical cursor col changes by more than 1 cell (word jump, line wrap).

## Putting it together — the feel

```c
// on each keypress that moves the cursor:
int old_col = app->cursor.log_col;
app->cursor.log_col = new_col;
app->cursor.log_row = new_row;

cursor_on_keypress(&app->cursor, now_ms());

// if jumped more than 1 cell, leave a trail
if (abs(new_col - old_col) > 1)
    trail_push(&app->trail, old_col, app->cursor.log_row, now_ms());

// each frame:
cursor_update(&app->cursor, dt);          // lerp/spring visual position
cursor_blink_update(&app->cursor, now);   // blink state
trail_render(&app->trail, win, theme, now);
render_cursor(win, &app->cursor, theme, now);
screen_composite(&screen, wins, nwins);
screen_flush(&screen);
```

The end result: cursor glides smoothly between cells, blinks during idle, stays solid while typing, leaves a brief warm trail on big jumps. No pixel rendering needed — it's entirely achieved through cell colors and timing.

