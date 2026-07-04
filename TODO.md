# cmonkey — Implementation Plan

This doc is a working map of what's left to build, in what order, and *why*.
Section 1 audits the codebase as it stands. Section 2 explains double
buffering from first principles. Section 3 is the concrete implementation
for `screen.c`. Section 4 covers the drawing components (boxes, word field,
cursor). Section 5 covers the test/input logic. Section 6 looks ahead to
UTF-8/multi-language support. Section 7 is the ordered roadmap.

---

## 1. Current state audit

| File | Status |
|---|---|
| `buffer.c/h` (`term_buf`) | Done. Raw byte buffer, appended to all frame, `write()`'d once via `tb_flush`. |
| `timer.c/h` | Done. Fixed-timestep frame pacing via `clock_nanosleep`. |
| `input.c/h` | Done. Non-blocking read, byte → `ACTION` mapping. |
| `config.c` | Mostly done. Two loose ends: (a) `text_fg`/`text_bg` should fall back to `main_fg`/`main_bg` when left blank in the theme file — currently the TODO comment says so but the code just `continue`s and leaves them as empty strings, which will emit broken/empty escape codes. (b) `config.h`'s `BORDER_CHARS` table has empty `{}` entries for `BORDER_BOLD` and `BORDER_DOUBLE`. |
| `draw.c/h` | Partial. `draw_text`, `draw_text_with_color`, `draw_box`, `draw_words_in_box` (the destructive/simple version) are implemented. `draw_words_in_box_ex` (the real word-field renderer with per-character coloring and scroll) is **declared but not defined anywhere** — currently commented out in `cmonkey_draw`. |
| `screen.c/h` | **Stub.** `screen.h` declares the full front/back buffer API (`screen_create`, `screen_destroy`, `screen_clear`, `screen_set`, `screen_flush`) but `screen.c` only contains an empty `screen_flush`. This is the double-buffering layer, and it's entirely unimplemented. |
| `wordbank.c/h` | Done. JSON load, arena-backed word storage, shuffle, queue refill. |
| `cmonkey.c/h` | Skeleton only for game logic: `cmonkey_test_new`, `test_refill_words`, and every branch of `cmonkey_handle_input` (char/backspace/del-word) are empty bodies. `cmonkey_update` has a TODO for topping up `cm->incoming` when low. `cmonkey_draw` has a TODO acknowledging it draws directly instead of through `screen`. The `CMONKEY_FINISHED` branch has a TODO for a real results screen. |

So there are really three independent tracks of work:

1. **Rendering pipeline** — build the double-buffer (`screen.c`) and rewire drawing through it instead of blasting raw escape codes + a full clear every frame.
2. **Drawing components** — finish `draw_words_in_box_ex`, fill in the missing border character sets, fix the theme fallback.
3. **Game logic** — test lifecycle, input handling, word queue refilling.

These are mostly independent, but it's worth doing (1) before finishing (2)'s word-field renderer, since where you write pixels/cells to matters (`screen_set` vs `tb_append_*` directly).

---

## 2. Double buffering — the concept

### The problem it solves

Right now, every single frame, `cmonkey_draw` does this:

```c
draw_clear(&cm->tb, &cm->t);   // \033[0m + colors, then \033[2J\033[H — wipes the ENTIRE terminal
Box border = {1, 1, cm->rows, cm->cols};
draw_box(&cm->tb, border, &cm->t, &cm->c);
// ...draw everything else, unconditionally...
tb_flush(&cm->tb);              // one write() of the whole frame
```

At 60 FPS that's 60 full-screen clears and full redraws a second, even if
*nothing changed* except, say, the timer digit in the corner. Two consequences:

- **Flicker.** `\033[2J` erases before the new frame's characters land. On a
  slow terminal emulator or over SSH, there's a visible blank flash before
  the border/text reappears — the "flicker" you're used to seeing described
  in game-dev docs.
- **Wasted bandwidth / CPU.** You're re-emitting box-drawing characters,
  color escapes, and cursor moves for cells that are pixel-for-pixel
  identical to what's already on screen.

### The fix, conceptually

Double buffering (in this terminal-UI context — not to be confused with GPU
double buffering, though the idea rhymes) means you keep **two logical
grids of cells**, not two literal video buffers:

- **`back` buffer** — what you *want* the screen to look like after this
  frame. All your `draw_*` calls during a frame write into this buffer, cell
  by cell (character + fg + bg + attributes). Nothing touches the terminal
  yet.
- **`front` buffer** — what the terminal *currently* displays, i.e., what
  you actually sent last frame.

At the end of the frame, you **diff** `back` against `front`, cell by cell.
For every cell that differs, you emit the minimal escape sequence to move
the cursor there and rewrite just that cell. Cells that are unchanged are
never touched — no clear, no cursor move, no re-print. Once the diff pass is
done, `back` becomes the new `front` (either by `memcpy` or by swapping
pointers and clearing the new `back` for next frame).

This is exactly the same idea as "dirty rectangle" rendering in old 2D
engines, or how `ncurses`' `wnoutrefresh`/`doupdate` works internally — you
maintain a model of "what should be shown" separately from "what has been
sent," and only the delta goes out the wire.

### Where `term_buf` fits in

`term_buf` doesn't go away — it's still the transport. The relationship is:

```
draw_* calls  →  screen_set(back buffer, cell data)   [in-memory, no I/O]
                          │
                    screen_flush()
                          │
              diff back vs front, for each dirty cell:
                 draw_move(term_buf, row, col)
                 draw_fg/draw_bg(term_buf, cell colors)
                 tb_append_n(term_buf, cell->ch, len)
                          │
                    tb_flush(term_buf)   →  one write() syscall
                          │
                 back buffer copied into front buffer
```

So `screen` is the *model* (what should be on screen), `term_buf` is the
*wire protocol buffer* (the actual bytes going to the terminal), and
`screen_flush` is the translator between them. You still get exactly one
`write()` per frame (good — syscalls are expensive), but the *contents* of
that write shrink to just the changed cells instead of the whole screen.

### First-frame problem: "poisoning" the front buffer

On the very first frame there is nothing on the real terminal yet, so
*everything* is dirty. The trick is to initialize `front` to a value that
can never legitimately match any real cell — e.g. `ch[0] = '\0'` with
`fg = NULL, bg = NULL`. Since a valid drawn cell always has `fg`/`bg` set to
a real (even if empty-string) theme pointer and a non-null character, this
sentinel guarantees the diff sees every cell as changed and does a full
draw on frame one. This is what "poison the front buffer" means in the
`screen.h` comment.

---

## 3. Implementing `screen.c`

Here's the concrete plan matching the API `screen.h` already declares.

### 3.1 `cell` equality

You'll compare cells constantly, so it's worth a small static helper first
(put it at the top of `screen.c`, not in the header — it's an
implementation detail):

```c
static bool cell_eq(const cell* a, const cell* b)
{
    return a->attr == b->attr
        && a->fg   == b->fg     // pointer compare: fg/bg point into cmonkey_theme, which is stable for the process lifetime
        && a->bg   == b->bg
        && memcmp(a->ch, b->ch, sizeof(a->ch)) == 0;
}
```

Note the pointer comparison for `fg`/`bg` is intentional and cheap — since
every color string lives inside the single long-lived `cmonkey_theme t`,
two cells drawn with "the theme's border color" will always share the exact
same pointer. You don't need `strcmp`.

### 3.2 `screen_create`

```c
void screen_create(screen* s, u32 rows, u32 cols)
{
    u64 n = (u64)rows * cols;
    s->front = malloc(n * sizeof(cell));
    s->back  = malloc(n * sizeof(cell));
    CHECK_FATAL(!s->front || !s->back, "screen alloc failed");

    s->rows = rows;
    s->cols = cols;

    // Poison front so the very first flush treats every cell as dirty.
    cell poison = {.ch = {0}, .fg = NULL, .bg = NULL, .attr = 0};
    for (u64 i = 0; i < n; i++) {
        s->front[i] = poison;
    }
    // back starts as plain spaces; screen_clear() will overwrite it
    // properly before the first real draw call anyway.
    memset(s->back, 0, n * sizeof(cell));
}
```

### 3.3 `screen_destroy`

```c
void screen_destroy(screen* s)
{
    if (!s) return;
    free(s->front);
    free(s->back);
    s->front = s->back = NULL;
}
```

### 3.4 `screen_clear`

Called once at the start of each frame, before any `draw_*` calls, to reset
the *back* buffer to theme background — analogous to what `draw_clear` does
today, but only touching the model, not the terminal:

```c
void screen_clear(screen* s, const cmonkey_theme* t)
{
    cell blank = {.ch = " ", .fg = t->text_fg, .bg = t->main_bg, .attr = 0};
    u64 n = (u64)s->rows * s->cols;
    for (u64 i = 0; i < n; i++) {
        s->back[i] = blank;
    }
}
```

### 3.5 `screen_set`

The single-cell write that every drawing routine will eventually call
instead of `tb_append_*` directly:

```c
void screen_set(screen* s, u32 row, u32 col,
                const char* ch, const char* fg, const char* bg)
{
    if (row >= s->rows || col >= s->cols) return; // clip, don't crash on resize races

    cell* c = &s->back[(u64)row * s->cols + col];
    strncpy(c->ch, ch, sizeof(c->ch) - 1);
    c->ch[sizeof(c->ch) - 1] = '\0';
    c->fg = fg;
    c->bg = bg;
    // attr intentionally left alone here; add a screen_set_attr() later
    // if/when bold/underline per-cell is needed.
}
```

### 3.6 `screen_flush` — the diff + emit + swap

This is the actual double-buffering step:

```c
void screen_flush(screen* s, term_buf* b, const cmonkey_theme* t)
{
    const char* cur_fg = NULL;
    const char* cur_bg = NULL;

    for (u32 r = 0; r < s->rows; r++) {
        for (u32 col = 0; col < s->cols; col++) {
            u64 i = (u64)r * s->cols + col;
            cell* bk = &s->back[i];
            cell* fr = &s->front[i];

            if (cell_eq(bk, fr)) continue; // unchanged — skip entirely

            draw_move(b, r + 1, col + 1); // terminal coords are 1-based

            if (bk->fg != cur_fg) { draw_fg(b, bk->fg ? bk->fg : t->text_fg); cur_fg = bk->fg; }
            if (bk->bg != cur_bg) { draw_bg(b, bk->bg ? bk->bg : t->text_bg); cur_bg = bk->bg; }

            tb_append_cstr(b, bk->ch[0] ? bk->ch : " ");

            *fr = *bk; // commit: front now matches what we just drew
        }
    }

    tb_flush(b);
}
```

Two optimizations worth calling out (nice-to-haves, not blockers):

- **Color-state tracking** (`cur_fg`/`cur_bg` above) avoids re-emitting an
  identical color escape for every single character in a run of
  same-colored dirty cells — you only pay for the escape when the color
  actually changes.
- **Run-length coalescing**: instead of one `draw_move` + one char per dirty
  cell, you could detect horizontal runs of *adjacent* dirty cells and emit
  one `draw_move` + all characters in the run at once (fewer cursor-move
  escapes). Skip this for v1; the color-state tracking above already
  removes most of the redundant bytes, and this is a pure perf polish you
  can add once the game loop is otherwise working.

### 3.7 Wiring it into `cmonkey`

- Add a `screen scr;` field to `cmonkey` (alongside `term_buf tb`).
- `cmonkey_create`: `screen_create(&cm->scr, cm->rows, cm->cols);` after
  `tb_create`.
- `cmonkey_destroy`: `screen_destroy(&cm->scr);`
- **Resize handling in `cmonkey_update`**: right now it does
  `tb_destroy`/`tb_create` on `resize_flag`. Do the equivalent for `scr`:
  `screen_destroy(&cm->scr); screen_create(&cm->scr, cm->rows, cm->cols);`
  — this re-poisons the front buffer, which is correct: after a resize the
  real terminal contents are unknown/invalid, so you *want* a full redraw.
- `cmonkey_draw`: replace `draw_clear(&cm->tb, ...)` with
  `screen_clear(&cm->scr, &cm->t)`, change every `draw_*` call to target the
  screen (see section 4), and end the function with
  `screen_flush(&cm->scr, &cm->tb, &cm->t)` instead of directly calling
  `tb_flush`.

---

## 4. Drawing components

### 4.1 `draw_box` — how it already works

Walking `draw_box` as written: it moves to `(box.r, box.c)`, sets the
border fg, picks the character set for the configured `BORDER_STYLE`, then
draws top-left corner, `w-2` horizontal segments, top-right corner, then for
each interior row moves to the left and right edge and drops a vertical
char, then finally draws the bottom row the same way the top was drawn, and
resets theme colors at the end. This part is solid.

**The gap**: `config.h`'s `BORDER_CHARS` table only has entries 0 and 1
filled in (`BORDER_SHARP`, `BORDER_ROUND`); `BORDER_BOLD` and
`BORDER_DOUBLE` are empty `{}`, which means selecting those styles in the
config will draw a box out of empty strings (invisible box). Fill them in:

```c
static const char BORDER_CHARS[4][6][8] = {
    {"┌", "┐", "└", "┘", "│", "─"},  // BORDER_SHARP
    {"╭", "╮", "╰", "╯", "│", "─"},  // BORDER_ROUND
    {"┏", "┓", "┗", "┛", "┃", "━"},  // BORDER_BOLD
    {"╔", "╗", "╚", "╝", "║", "═"},  // BORDER_DOUBLE
};
```

**Once double buffering is wired in**, `draw_box` should stop calling
`tb_append_cstr`/`draw_move` directly and instead call `screen_set(scr, row,
col, glyph, t->border, t->main_bg)` per cell — same logic, different sink.
Practically: change its signature to take a `screen*` instead of
`term_buf*`, and replace each `draw_move` + `tb_append_cstr` pair with a
single `screen_set` call at that coordinate. The loop structure doesn't
change at all, only where the character lands.

### 4.2 Theme fallback for `text_fg`/`text_bg`

In `theme_load`, blank `text_fg`/`text_bg` values are currently just
skipped, per the TODO. Since `cmonkey_theme` fields are fixed-size char
arrays and start zeroed, an unset `text_fg` is an empty string — passing
that to `draw_fg`/`screen_set` emits nothing, which silently inherits
whatever color was last active rather than the intended "same as main"
behavior. Fix by defaulting after the parse loop, once both values are
known:

```c
// after the while(fgets...) loop, before building t->reset:
if (t->text_fg[0] == '\0') { memcpy(t->text_fg, t->main_fg, sizeof(t->text_fg)); }
if (t->text_bg[0] == '\0') { memcpy(t->text_bg, t->main_bg, sizeof(t->text_bg)); }
```

### 4.3 `draw_words_in_box_ex` — the real word-field renderer

This is the biggest undone piece of `draw.c`. Per the spec comment already
in `draw.h`, it needs to render `test->words[]` starting at
`test->typed_base`, coloring committed words by their state, the current
word character-by-character, and upcoming words dimmed — then keep
`curr_word` bottom-anchored via scroll.

Suggested implementation shape:

```c
void draw_words_in_box_ex(term_buf* b, WordBank* wb, Box box,
                          cmonkey_test* test, const cmonkey_theme* t)
{
    u32 inner_w = box.w - 2;
    u32 inner_h = box.h - 2;
    u32 line = 0, col = 0;     // relative to box interior
    u32 first_visible = test->typed_base;

    // Pass 1: lay words out starting at typed_base, wrapping on inner_w,
    // and if curr_word's line would exceed inner_h, advance typed_base
    // to the start of the *next* line and redo the layout (bottom-anchor
    // scroll). This is the same wrapping loop as draw_words_in_box, just
    // non-destructive (reads test->words by index, doesn't dequeue).
    //
    // Practical approach: do a first dry-run pass computing which line
    // curr_word lands on; if it's >= inner_h, bump typed_base forward by
    // one line's worth of words and repeat, then do the real draw pass.

    for (u32 i = first_visible; i < test->words.size; i++) {
        Word* w = (Word*)genVec_get_ptr(&test->words, i);
        // WORD_STATE stored alongside idx — extend Word or keep a
        // parallel array/genVec of WORD_STATE if Word can't grow.

        if (col + w->len >= inner_w) { line++; col = 0; }
        if (line >= inner_h) break; // don't draw past the box

        const char* color =
            (i <  test->curr_word) ? (state_of(i) == WORD_CORRECT ? t->correct : t->incorrect) :
            (i == test->curr_word) ? NULL /* char-by-char below */ :
                                      t->text_dim;

        if (i == test->curr_word) {
            const char* word_str = wordbank_word_at(wb, w->idx);
            for (u32 ch_i = 0; ch_i < w->len; ch_i++) {
                const char* c =
                    (ch_i < test->curr_typed_len)
                        ? (test->curr_typed[ch_i] == word_str[ch_i] ? t->correct : t->incorrect)
                        : (ch_i == test->curr_typed_len ? t->cursor : t->text_dim);
                draw_move(b, box.r + 1 + line, box.c + 1 + col + ch_i);
                draw_fg(b, c);
                tb_append_n(b, word_str + ch_i, 1);
            }
        } else {
            draw_move(b, box.r + 1 + line, box.c + 1 + col);
            draw_fg(b, color);
            tb_append_n(b, wordbank_word_at(wb, w->idx), w->len);
        }

        col += w->len + 1;
    }

    draw_theme_reset(b, t);
}
```

Notes:
- `test->words` currently only stores `Word {idx, len}` via whatever genVec
  element type is chosen at `genVec_init` time — you'll need somewhere to
  keep `WORD_STATE` per committed word (either widen the element struct
  pushed into `test->words` to `{idx, state}` as the comment on
  `cmonkey_test.words` already says, or track a parallel small array sized
  to `WORDS_AHEAD`).
- The "bump `typed_base` until `curr_word` fits" scroll logic is the part
  most worth writing a quick unit test for, since off-by-one errors there
  are exactly the kind of thing that looks fine until a line wraps at the
  edge.
- Once `screen`/double buffering is in place, this function should take a
  `screen*` and call `screen_set` per character instead of
  `draw_move`+`draw_fg`+`tb_append_n`, same transformation as `draw_box`.

### 4.4 Cursor rendering

`cmonkey_conf` already parses `cursor_style` (`CURSOR_BAR`, `CURSOR_BLOCK`,
`CURSOR_UNDERLINE`) out of the config file, but nothing draws it yet — the
pseudocode in 4.3 just used `t->cursor` as a plain foreground color, which
is a start but doesn't actually distinguish the three styles. This section
finishes that.

**Why the terminal's real cursor isn't used.** `cmonkey_init_term` emits
`CURSOR_HIDE` on purpose — with a custom-rendered UI, you want the "cursor"
to be a themed, stylable thing that lives inside your own cell grid, not
whatever blink rate/shape the user's terminal emulator happens to default
to. So the cursor has to be drawn as regular cell content, same as any
letter or border piece.

**Where the cursor sits.** It's always at
`(curr_word's line, curr_word's column + curr_typed_len)` — i.e. on top of
the *next untyped character* of the current word, or one column past the
last character if `curr_typed_len == w->len` (the user has fully typed the
word but hasn't hit space yet). That second case matters: you need to draw
the cursor on a *space* cell past the word, which means the loop in 4.3
must run one extra iteration for the current word when
`curr_typed_len == w->len`.

**How each style renders as one cell's attributes:**

| Style | Effect | How |
|---|---|---|
| `CURSOR_BLOCK` | The whole character cell appears as a solid block in the cursor color | Reverse video: swap fg/bg on that cell (`\033[7m`), or just set the cell's `bg` to `t->cursor` and `fg` to `t->main_bg` directly — simpler and avoids depending on the terminal's reverse-video support |
| `CURSOR_UNDERLINE` | A line under the character | Underline attribute on that cell (`\033[4m` before, `\033[24m` after) — the character itself keeps its normal fg/bg |
| `CURSOR_BAR` | A thin vertical line, like an I-beam, at the *left edge* of the cell | See note below — this one doesn't fit the "recolor one cell" model as cleanly |

**The bar-cursor wrinkle.** Terminal cells are indivisible glyph slots —
you can't natively draw "a line down the left third of this cell" and
still show the letter that belongs there, because there's no sub-cell
compositing in plain ANSI text. Two practical options, in order of how
well they preserve readability:

1. **Recommended for v1**: draw the bar cursor as its own one-eighth-block
   character (`▏`, U+258F) *instead of* the character underneath it, in
   `t->cursor` color, and just accept that the letter under the cursor is
   temporarily hidden — this is visually exactly how a bar cursor looks in
   a text editor before you type over it, and it's zero extra layout work
   since it's still one cell, one glyph, no width change.
2. **Fancier, skip for v1**: reserve a *between-character* gap — i.e. change
   word layout so there's a spare column between every pair of characters
   in the current word only, and draw `▏` in that gap column, leaving both
   neighboring letters fully visible. This changes column math only for
   the current word (not committed/upcoming ones), which is a nice
   MonkeyType-like touch, but it's extra layout complexity worth doing only
   after everything else in this doc is working.

**Extending the `cell`/`screen` model to carry this.** `screen.h`'s `cell`
already has an `attr` byte for exactly this purpose, but `screen_flush`
today only diffs and emits fg/bg — it never reads or emits `attr`. Define
a small bitmask (put it in `screen.h` next to `cell`):

```c
enum {
    CELL_ATTR_NONE      = 0,
    CELL_ATTR_UNDERLINE = 1 << 0,
    CELL_ATTR_REVERSE   = 1 << 1, // available if you want the classic \033[7m block cursor instead of recoloring
};
```

Then in `screen_flush`, track a `cur_attr` alongside `cur_fg`/`cur_bg` and
emit the on/off escapes when it changes:

```c
u8 cur_attr = CELL_ATTR_NONE;
// ...inside the per-cell loop, after the fg/bg checks...
if (bk->attr != cur_attr) {
    if ((bk->attr & CELL_ATTR_UNDERLINE) && !(cur_attr & CELL_ATTR_UNDERLINE)) { tb_append_cstr(b, UNDERLINE_ON); }
    if (!(bk->attr & CELL_ATTR_UNDERLINE) && (cur_attr & CELL_ATTR_UNDERLINE)) { tb_append_cstr(b, UNDERLINE_OFF); }
    cur_attr = bk->attr;
}
```

(Same pattern for `CELL_ATTR_REVERSE` with `\033[7m`/`\033[27m` if you go
that route for the block cursor instead of direct fg/bg recoloring.)

**Wiring it into `draw_words_in_box_ex`.** At the point in the per-character
loop where `ch_i == test->curr_typed_len` (the cursor position), branch on
`c->cursor_style` instead of just picking `t->cursor` as a color:

```c
if (ch_i == test->curr_typed_len) {
    switch (conf->cursor_style) {
    case CURSOR_BLOCK:
        screen_set(scr, row, col, &word_str[ch_i], t->main_bg, t->cursor);
        break;
    case CURSOR_UNDERLINE:
        screen_set(scr, row, col, &word_str[ch_i], t->cursor, t->text_bg);
        screen_set_attr(scr, row, col, CELL_ATTR_UNDERLINE); // small helper, or fold into screen_set
        break;
    case CURSOR_BAR:
        screen_set(scr, row, col, "▏", t->cursor, t->text_bg);
        break;
    }
}
```

(`screen_set_attr` isn't in the current `screen.h` — either add it as a
one-line sibling to `screen_set` that only touches `cell.attr`, or widen
`screen_set`'s signature to take an `attr` param and pass `0` from every
other call site. Either is fine; the sibling-function version touches less
existing code.)

This is entirely an English/Latin-alphabet-safe design as written — one
`char` per cell, one column per character. Section 6 covers what changes
once non-ASCII text enters the picture.

---

## 5. Test lifecycle & input logic

### 5.1 `cmonkey_test_new`

Called on startup and on Tab (`ACTION_RESTART`). Needs to:

```c
void cmonkey_test_new(cmonkey* cm)
{
    if (cm->test.words) { genVec_destroy_stk(&cm->test.words); }
    cm->test.words = genVec_init_stk(WORDS_AHEAD * 2, sizeof(Word), NULL);

    cm->test.elapsed_time    = 0.f;
    cm->test.typed_base      = 0;
    cm->test.curr_word       = 0;
    cm->test.curr_char       = 0;
    cm->test.correct         = 0;
    cm->test.incorrect       = 0;
    cm->test.curr_typed_len  = 0;
    cm->test.curr_typed[0]   = '\0';

    cm->state = CMONKEY_WAITING;

    test_refill_words(cm); // pull WORDS_AHEAD words out of cm->incoming into test.words
}
```

(Match `genVec_destroy_stk`/`genVec_init_stk` to whichever variant
`cmonkey.h`'s `genVec_destroy_stk` counterpart actually is in your vector
lib — the header only shows `genVec_destroy_stk` being called in
`cmonkey_destroy`, so use the matching create call.)

### 5.2 `test_refill_words`

```c
static void test_refill_words(cmonkey* cm)
{
    u32 have = (u32)cm->test.words->size - cm->test.curr_word;
    while (have < WORDS_AHEAD) {
        if (queue_is_empty(&cm->incoming)) {
            wordbank_random_words_in_queue(&cm->wb, &cm->incoming);
        }
        u32 idx = DEQUEUE(&cm->incoming, u32);
        Word* w = (Word*)genVec_get_ptr(cm->wb.words, idx);
        genVec_push(cm->test.words, (u8*)w);
        have++;
    }
}
```

Call this from `cmonkey_update` every frame (cheap early-return since
`have < WORDS_AHEAD` is false most frames) — this is exactly the "TODO: get
more words in cm->incoming queue if low" note in `cmonkey_update`, and the
"get more words in test->word[] genvec if less than WORDS_AHEAD remain"
TODO right under it. One function, two responsibilities: keep the wordbank
queue topped up, and keep the on-screen word list topped up from that
queue.

### 5.3 `cmonkey_handle_input`

```c
case ACTION_CHAR:
    if (cm->state == CMONKEY_WAITING) {
        cm->state = CMONKEY_UNDERGOING;
    }
    if (cm->state == CMONKEY_UNDERGOING) {
        if (input.ch == ' ') {
            // commit current word
            Word* w = (Word*)genVec_get_ptr(cm->test.words, cm->test.curr_word);
            const char* correct_str = wordbank_word_at(&cm->wb, w->idx);
            bool ok = (cm->test.curr_typed_len == w->len) &&
                      (strncmp(cm->test.curr_typed, correct_str, w->len) == 0);
            ok ? cm->test.correct++ : cm->test.incorrect++;
            cm->test.curr_word++;
            cm->test.curr_typed_len = 0;
            cm->test.curr_typed[0] = '\0';
            test_refill_words(cm);
        } else if (cm->test.curr_typed_len < MAX_TYPED_SIZE - 1) {
            cm->test.curr_typed[cm->test.curr_typed_len++] = input.ch;
            cm->test.curr_typed[cm->test.curr_typed_len] = '\0';
        }
    }
    break;

case ACTION_BACKSPACE:
    if (cm->test.curr_typed_len > 0) {
        cm->test.curr_typed[--cm->test.curr_typed_len] = '\0';
    }
    break;

case ACTION_DEL_WORD:
    cm->test.curr_typed_len = 0;
    cm->test.curr_typed[0] = '\0';
    break;
```

A judgment call for you: whether a space with zero typed characters should
count as an (incorrect, empty) submission or be ignored — MonkeyType-style
testers usually ignore a bare space on an empty word. Guard `if
(cm->test.curr_typed_len > 0)` around the space-commit branch if you want
that behavior.

### 5.4 Results screen

The `CMONKEY_FINISHED` TODO is a good place to show WPM
(`correct / (elapsed_time / 60)`) and accuracy
(`correct / (correct + incorrect)`), using `draw_text`/`draw_box` exactly
like the rest of the UI — no new drawing primitives needed there, just
composition of what already exists once `draw_words_in_box_ex` and the
double buffer are done.

---

## 6. Future: UTF-8 & multi-language support

Not needed for the English-only v1, but worth understanding now since it
touches nearly every layer you're about to build — better to know the shape
of it before you bake in assumptions that make it painful later. `wordbank.h`
already has a TODO acknowledging this ("ARABIC is loading correctly with
setlocale(), idk about cursor movement"), and `screen.h`'s `cell.ch` is
already sized `char[4]`, which is exactly one UTF-8 code point's max byte
length — a good sign someone was already thinking about this. Everything
below is the rest of what that decision implies.

### 6.1 The core distinction: bytes vs. characters vs. display columns

This is the thing that will bite you first. Right now, code like
`Word.len`, `curr_typed_len`, and the loop bound `ch_i < w->len` all quietly
assume **1 byte = 1 character = 1 terminal column**. That's only true for
ASCII. Once you support UTF-8:

- **Bytes**: a single character can be 1–4 bytes (`a` = 1 byte, `é` = 2
  bytes, `字` = 3 bytes, some emoji = 4 bytes).
- **Characters (code points)**: what a person thinks of as "one letter."
- **Display columns**: how many terminal cells wide it renders. Most
  Latin/Cyrillic/Arabic/Hebrew characters are 1 column. CJK (Chinese,
  Japanese, Korean) characters are typically **2 columns wide** —
  "fullwidth" — even though they're one character and 3 bytes. This is
  governed by the Unicode East Asian Width property, and the standard way
  to query it in C is `wcwidth()` (POSIX, `<wchar.h>`) after converting a
  UTF-8 sequence to a wide char.

Concretely: `Word.len` (currently "byte length, which happens to equal
column width for English") needs to become two numbers once other scripts
are in play — byte length (for `memcpy`/arena storage) and column width
(for `draw_words_in_box_ex`'s wrapping math). Keeping both is cheap; it's
computing column width once at word-load time (via `wcwidth` per code
point, summed) and caching it in the `Word` struct that costs the design
work.

### 6.2 Input: decoding multi-byte sequences

`input.c`'s `process_char` takes one `unsigned char` at a time and only
recognizes `ACTION_CHAR` for the ASCII printable range (32–126). A UTF-8
continuation byte (0x80–0xBF) or a lead byte (0xC0+) will simply fall
through and be dropped today.

The fix is a small decode state machine sitting in front of (or inside)
`input_read_all`:

1. Look at the first byte of a potential character:
   - `0xxxxxxx` → 1-byte ASCII, handled exactly as today.
   - `110xxxxx` → 2-byte sequence, need 1 more continuation byte.
   - `1110xxxx` → 3-byte sequence, need 2 more.
   - `11110xxx` → 4-byte sequence, need 3 more.
2. Accumulate continuation bytes (`10xxxxxx` pattern) until the sequence is
   complete, buffering them in a small local array.
3. Emit one `ACTION_CHAR` event carrying the *whole* code point, not one
   byte — which means `cmonkey_input.ch` (currently a single `char`) needs
   to become a small fixed buffer, e.g. `char ch[4]` mirroring `cell.ch`,
   plus a length byte.
4. Handle the edge case where a multi-byte sequence is split across two
   `read()` calls (very possible with non-blocking I/O and fast typing) —
   you'll need a small piece of persistent state (a partial-sequence
   buffer) that survives between calls to `input_read_all`, not just local
   variables.

### 6.3 Locale setup

For `wcwidth()` (and for the Arabic wordbank loading mentioned in the
`wordbank.h` TODO) to behave correctly, the process needs a UTF-8 locale
active — `setlocale(LC_ALL, "")` early in `main()`, before any wide-char
functions are called, picking up the user's `LANG`/`LC_ALL` environment.
Without this, `wcwidth()` on non-ASCII input is undefined/wrong on many
libc implementations.

### 6.4 Rendering: `cell.ch` and column math

- `cell.ch[4]` already has room for one UTF-8 code point per cell — good.
  But a 2-column-wide character (CJK) occupies **one `cell` struct with
  content, followed by one empty/"continuation" cell that must be skipped**
  during diffing and cursor placement, otherwise column math throughout
  `draw_box`/`draw_words_in_box_ex` (which currently assumes `1 char = 1
  column`) drifts by one column per wide character. The common convention
  (also used by `ncurses`' wide-char mode) is to mark the second cell of a
  wide character with a sentinel (e.g. `ch[0] = '\0'` with a dedicated
  "this is a continuation, don't touch me" flag) so the diff/flush loop in
  `screen_flush` knows to skip drawing it but still counts it when moving
  the cursor.
- Every place that currently does `box.c + 1 + col` / `col += w->len + 1`
  needs to use accumulated **column width**, not byte length or code-point
  count.

### 6.5 Bidirectional text (Arabic/Hebrew) — the hard part, explicitly out of scope for now

Right-to-left scripts aren't just "reverse the string" — real-world text
mixes RTL words with embedded LTR runs (numbers, Latin brand names), which
requires the Unicode Bidirectional Algorithm (UAX #9) to lay out correctly.
This is a substantial chunk of work on its own (typically pulled in via a
library like ICU's bidi support or a minimal from-scratch UAX #9
subset) and is **not** solved by `wcwidth`/UTF-8 decoding alone. Treat
UTF-8 support (6.1–6.4) and bidi support (this section) as two separate
milestones — the wordbank TODO's uncertainty about Arabic cursor movement
is specifically pointing at this gap. Realistic scope for "future support"
is: UTF-8 decode + `wcwidth`-correct CJK layout first, RTL scripts as a
clearly-separate follow-up milestone rather than something to half-solve
alongside everything else in this doc.

### 6.6 Summary of what to keep in mind now (even before implementing any of this)

You don't need to build any of section 6 today, but two small habits now
will save a rewrite later:

- Don't let `Word.len` / `curr_typed_len` semantics get baked in as "array
  index == column position" anywhere outside `draw_words_in_box_ex` itself
  — keep that assumption localized to one function so it's a smaller patch
  later.
- Keep `cell.ch` as a small fixed buffer (already true) rather than a
  single `char`, and keep `cmonkey_input.ch` heading the same direction
  when you touch `input.h` for the cursor/word-commit work in section 5 —
  it costs nothing today and means section 6.2 doesn't require touching
  every call site again.

---

## 7. Roadmap — suggested order

1. **Fill in `BORDER_BOLD`/`BORDER_DOUBLE`** in `config.h` (5 minutes, unblocks nothing else but is a quick win).
2. **Fix the `text_fg`/`text_bg` fallback** in `config.c`'s `theme_load`.
3. **Implement `screen.c`** as in section 3, including the `attr` bitmask handling from 4.4 — this is the biggest structural change and everything downstream benefits from it being in place first.
4. **Refactor `draw_box`, `draw_text`, `draw_text_with_color`** to draw into a `screen*` via `screen_set` instead of writing raw escapes into `term_buf` directly. Update `cmonkey_draw` to call `screen_clear`/`screen_flush` instead of `draw_clear`/`tb_flush`.
5. **Implement `draw_words_in_box_ex`** (section 4.3) plus cursor rendering (section 4.4), targeting the screen API from the start rather than writing it against raw `term_buf` and refactoring twice.
6. **Implement `cmonkey_test_new` and `test_refill_words`** (section 5.1–5.2).
7. **Implement `cmonkey_handle_input`**'s char/backspace/del-word branches (section 5.3).
8. **Wire the incoming-queue refill** into `cmonkey_update` (already covered by calling `test_refill_words` each frame, per 5.2).
9. **Build the `CMONKEY_FINISHED` results screen** (section 5.4).
10. **Polish pass**: color-state tracking / run-length coalescing in `screen_flush` (section 3.6) if frame bandwidth or terminal flicker is still noticeable once everything above is live.
11. **(Later milestone, not blocking v1)**: UTF-8 input decoding + `wcwidth`-aware layout (section 6.1–6.4), then bidi/RTL support as its own separate milestone after that (section 6.5).

Steps 1–2 and 6–8 can be done in any order relative to each other; steps
3–5 are sequential (each depends on the previous). Doing the rendering
refactor (3–5) before the game-logic work (6–9) means you only write
`draw_words_in_box_ex` once, against the final API, instead of writing it
against `term_buf` and then having to port it to `screen` later. Step 11 is
deliberately last and separate — it's a substantial effort on its own and
has no dependency on the English-only v1 being "wrong," just incomplete.
