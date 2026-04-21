# cmonkey — project architecture

## project layout

```
cmonkey/
├── include/
│   ├── term_buf.h        # string builder / frame buffer
│   ├── cell.h            # cell_t — the atomic unit of the screen
│   ├── screen.h          # screen_t — front/back buffers + differ
│   ├── window.h          # window_t — clipping regions / panels
│   ├── draw.h            # raw ANSI emit functions
│   ├── theme.h           # theme_t, rgb_t, border styles
│   ├── cursor.h          # cursor_t — smooth movement, blink, trail, shape
│   ├── input.h           # key_event_t, key parser
│   ├── config.h          # config loading from JSON
│   ├── time.h            # handeling timer for test, cursor, FPS
│   └── widgets.h         # border, progress bar, word display, status
│
├── src/
│   ├── main.c            # entry point, game loop
│   ├── term_buf.c        # tb_init, tb_append, tb_appendf, tb_flush
│   ├── screen.c          # screen_init, screen_resize, screen_flush (differ)
│   ├── window.c          # win_put, win_print, win_resize, compositor
│   ├── draw.c            # draw_move, draw_fg, draw_bg, draw_reset …
│   ├── cursor.c          # cursor_update, cursor_blink, trail
│   ├── input.c           # term_read_key, key sequence state machine
│   ├── config.c          # config_load, config_free (jsmn JSON parser)
│   ├── widgets.c         # draw_border, draw_word, draw_progress …
│   └── utf8.c            # grapheme cluster iteration, display width
│
├── themes/
│   ├── catppuccin_mocha.json
│   ├── gruvbox_dark.json
│   ├── nord.json
│   └── default.json
│
├── config.json           # user config (theme path, border style, etc.)
├── vendor/
│   ├── jsmn.h            # JSON tokeniser (single header)
│   └── utf8proc/         # grapheme cluster / wcwidth
│
└── Makefile
```

---

## key structures

### `rgb_t` — a color
```c
// include/theme.h
typedef struct {
    uint8_t r, g, b;
} rgb_t;

static inline rgb_t rgb_hex(uint32_t hex) {
    return (rgb_t){ (hex>>16)&0xFF, (hex>>8)&0xFF, hex&0xFF };
}

static inline rgb_t rgb_lerp(rgb_t a, rgb_t b, float t) {
    return (rgb_t){
        (uint8_t)(a.r + (b.r-a.r)*t),
        (uint8_t)(a.g + (b.g-a.g)*t),
        (uint8_t)(a.b + (b.b-a.b)*t),
    };
}
```
**Responsibility:** the only color representation in the whole project. Everything else holds `rgb_t`, never raw ints or palette indices.

---

### `theme_t` — named color slots + style
```c
// include/theme.h
typedef enum { BORDER_SHARP=0, BORDER_ROUNDED, BORDER_BOLD, BORDER_DOUBLE } border_style_t;

typedef struct {
    rgb_t bg, fg, dim;
    rgb_t accent;
    rgb_t correct, wrong;
    rgb_t border_normal, border_focused, border_title;
    rgb_t status_bar_bg, status_bar_fg;
    border_style_t border_style;
    char name[64];
} theme_t;
```
**Responsibility:** the single source of truth for all visual decisions. Loaded from JSON at startup. All widgets receive a `const theme_t *` — nothing hardcodes a color.

---

### `term_buf_t` — the frame buffer
```c
// include/term_buf.h
typedef struct {
    char  *data;
    size_t len;
    size_t cap;
} term_buf_t;

void tb_init   (term_buf_t *b, size_t cap);
void tb_free   (term_buf_t *b);
void tb_append (term_buf_t *b, const char *s);
void tb_appendf(term_buf_t *b, const char *fmt, ...);  // printf-style
void tb_flush  (term_buf_t *b);   // single write() to STDOUT_FILENO
void tb_resize (term_buf_t *b, size_t new_cap);
```
**Responsibility:** accumulate ANSI byte strings during a frame without any syscalls. `tb_flush` is the *only* place that touches stdout. One allocation at startup, sized to `rows * cols * 64 + 1024` bytes. Never reallocates during a frame.

---

### `cell_t` — one terminal character slot
```c
// include/cell.h
#define ATTR_BOLD      (1<<0)
#define ATTR_DIM       (1<<1)
#define ATTR_ITALIC    (1<<2)
#define ATTR_UNDERLINE (1<<3)
#define ATTR_REVERSE   (1<<4)

typedef struct {
    char    ch[5];      // UTF-8 grapheme (up to 4 bytes + NUL)
    rgb_t   fg, bg;
    uint8_t attrs;
    uint8_t wide;       // 1 if this is a 2-column wide char (CJK)
} cell_t;
```
**Responsibility:** the atomic unit of the screen. Every window and the screen itself is a flat array of `cell_t`. Comparing two `cell_t` (via `memcmp`) is how the differ decides what to skip.

---

### `screen_t` — the double buffer + differ
```c
// include/screen.h
typedef struct {
    int      rows, cols;
    cell_t  *front;     // what the terminal currently shows
    cell_t  *back;      // what we're building this frame
    term_buf_t emit;    // ANSI output — sized at init, never realloced
} screen_t;

void screen_init     (screen_t *s, int rows, int cols);
void screen_resize   (screen_t *s, int rows, int cols);  // on SIGWINCH
void screen_composite(screen_t *s, window_t **wins, int n); // blit windows → back
void screen_flush    (screen_t *s);  // diff front vs back → emit → swap
```
**Responsibility:** owns the two full-screen cell grids. `screen_flush` is the differ — it walks every cell, skips unchanged ones, tracks current SGR state to avoid redundant color codes, emits only what changed, then syncs front ← back. This is the performance core of the whole renderer.

Buffer sizing rule:
```c
// worst case: 64 bytes of ANSI per cell
size_t screen_buf_size(int rows, int cols) {
    return (size_t)rows * cols * 64 + 1024;
}
```

---

### `window_t` — a panel / clipping region
```c
// include/window.h
typedef struct {
    int     row, col;       // top-left on screen (0-indexed)
    int     w, h;
    int     z;              // compositing order — higher = on top
    int     visible;
    int     dirty;          // needs re-render this frame
    cell_t *cells;          // w * h cell buffer (owned)
    char   *title;
} window_t;

window_t *win_create (int row, int col, int w, int h, int z);
void      win_destroy(window_t *w);
void      win_resize (window_t *w, int row, int col, int nw, int nh);
void      win_clear  (window_t *w, rgb_t bg);
void      win_put    (window_t *w, int row, int col, cell_t cell);
void      win_print  (window_t *w, int row, int col,
                      const char *utf8, rgb_t fg, rgb_t bg, uint8_t attrs);
```
**Responsibility:** a self-contained cell buffer with a position and z-order. Widgets write cells into windows via `win_put` / `win_print`. The screen compositor blits all visible windows onto `screen.back` in z-order (painter's algorithm). Popups are just high-z windows created on demand.

---

### `cursor_t` — smooth animated cursor
```c
// include/cursor.h
typedef struct {
    // logical position — where the cursor actually is
    int     log_col, log_row;

    // visual position — where it's rendered (float for lerp)
    float   vis_col, vis_row;

    // spring physics state
    float   vel_col, vel_row;

    // blink
    uint64_t blink_timer_ms;
    int      blink_on;
    int      blink_period_ms;   // default 530

    // trail
    struct {
        int      col, row;
        uint64_t born_ms;
    } trail[4];
    int trail_head;
} cursor_t;

void cursor_move    (cursor_t *c, int col, int row, uint64_t now_ms);
void cursor_update  (cursor_t *c, float dt_sec);   // spring + blink
void cursor_render  (cursor_t *c, window_t *win, const theme_t *t, uint64_t now_ms);
```
**Responsibility:** decouples *logical* cursor position (set instantly on keypress) from *visual* cursor position (interpolated each frame via spring physics). Blink resets to solid on every keypress. Trail cells fade out over ~150ms after large jumps.

---

### `key_event_t` — normalised input
```c
// include/input.h
typedef enum {
    KEY_NONE=0, KEY_CHAR,
    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
    KEY_CTRL_UP, KEY_CTRL_DOWN, KEY_CTRL_LEFT, KEY_CTRL_RIGHT,
    KEY_BACKSPACE, KEY_DELETE, KEY_ENTER, KEY_ESCAPE, KEY_TAB,
} key_type_t;
// TODO: ctrl-backspace to delete last word

#define MOD_SHIFT (1<<0)
#define MOD_CTRL  (1<<1)
#define MOD_ALT   (1<<2)

typedef struct {
    key_type_t type;
    char       ch[5];    // UTF-8 grapheme for KEY_CHAR
    uint8_t    mods;
} key_event_t;

int term_read_key(key_event_t *out);   // non-blocking, returns 0 if no input
```
**Responsibility:** hides the raw byte sequences that terminals emit for special keys. A state machine reads stdin, waits up to 50ms after a lone `\033` to distinguish Escape from a sequence start, then maps to `key_event_t`. The rest of the app never sees raw bytes.

---

### `config_t` — user settings
```c
// include/config.h
typedef struct {
    // char theme_path[256];      // path to a theme JSON file
    int  show_wpm;
    int  show_progress;
    int  smooth_cursor;
    int  cursor_blink;
    int  cursor_blink_ms;      // blink period
    // TODO: cursor type

    // char word_list[256];       // path to word list file
    int  word_count;           // words per round
    int  time_limit_sec;       // 0 = no limit
} config_t;

int  config_load(config_t *cfg, const char *path);  // returns 0 on success
void config_free(config_t *cfg);
```
**Responsibility:** parsed once at startup from `config.json` using jsmn. Passed as `const config_t *` everywhere. Theme is loaded separately via `config.theme_path`.

---

## file responsibilities

| file | owns | does |
|---|---|---|
| `term_buf.c` | heap char buffer | append strings, flush to stdout |
| `screen.c` | front + back cell grids | differ, composite, resize |
| `window.c` | per-window cell buffers | write cells, blit to screen |
| `draw.c` | nothing (stateless) | emit ANSI sequences into a `term_buf_t` |
| `cursor.c` | cursor state | spring physics, blink, trail render |
| `input.c` | stdin state machine | raw bytes → `key_event_t` |
| `config.c` | parsed config + theme | JSON → `config_t` + `theme_t` |
| `widgets.c` | nothing (stateless) | draw border, word display, progress bar into a window |
| `utf8.c` | nothing (stateless) | grapheme iteration, display width |
| `main.c` | app state, game loop | init, SIGWINCH, input dispatch, frame timing |

---

## data flow — one frame

```
main loop
  │
  ├─ check g_resized flag (SIGWINCH)
  │     └─ screen_resize + layout_reflow
  │
  ├─ handle_input → key_event_t
  │     ├─ cursor_move (updates log_col/row, pushes trail)
  │     └─ game logic (update char states: correct/wrong/upcoming)
  │
  ├─ cursor_update(dt)        — spring physics, blink timer
  │
  ├─ for each dirty window:
  │     widget_render(win, theme)    — win_print cells into window buffer
  │     cursor_render(cursor, win)   — overlay cursor cell
  │
  ├─ screen_composite(screen, wins, n)   — blit windows → screen.back
  │
  ├─ screen_flush(screen)
  │     ├─ diff back vs front cell by cell
  │     ├─ emit ANSI into screen.emit (term_buf_t)
  │     ├─ tb_flush → single write()
  │     └─ sync front ← back
  │
  └─ nanosleep(remaining 16ms budget)
```

---

## config.json format

```json
{
    "theme": "themes/catppuccin_mocha.json",
    "word_list": "words/english_1k.txt",
    "word_count": 50,
    "time_limit_sec": 60,
    "show_wpm": true,
    "show_progress": true,
    "smooth_cursor": true,
    "cursor_blink": true,
    "cursor_blink_ms": 530
}
```

## theme JSON format

```json
{
    "name": "Catppuccin Mocha",
    "border": "rounded",
    "colors": {
        "bg":             "#1e1e2e",
        "fg":             "#cdd6f4",
        "dim":            "#585b70",
        "accent":         "#cba6f7",
        "correct":        "#a6e3a1",
        "wrong":          "#f38ba8",
        "border_normal":  "#585b70",
        "border_focused": "#cba6f7",
        "border_title":   "#cdd6f4",
        "status_bar_bg":  "#181825",
        "status_bar_fg":  "#a6adc8"
    }
}
```

Parsing a hex color with jsmn:
```c
// after extracting the string token value e.g. "#cba6f7"
static rgb_t parse_hex(const char *s) {
    if (*s == '#') s++;
    uint32_t v = (uint32_t)strtol(s, NULL, 16);
    return rgb_hex(v);
}
```

---

## on config format choice

**JSON + jsmn** is fine if it's already a dependency. The main limitation is no comments — mitigate by shipping well-documented example theme files and a README.

**Alternatives worth knowing:**

- `key = value` (hand-rolled, ~50 lines) — friendliest for users, supports `# comments`, trivial to parse, but no nesting so theme must be a separate file anyway
- TOML — best user experience, supports comments and nested tables, but needs a TOML parser dependency
- JSON — fine for machine-generated or developer-edited configs, no comments

For this project the two-file split (a `config.json` for settings + separate theme JSON files) is the right structure regardless of format. Keep them separate so users can share theme files independently of their personal config.
