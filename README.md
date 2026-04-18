
# Monkeytype-style Typing TUI in C — Clean Modular Project Structure

build full frame → compare → write only changes

## Project Layout

```text
monkeytui/
│
├── src/
│   ├── main.c            # program entry, app loop
│   │
│   ├── app.c             # app state + high-level logic
│   ├── app.h
│   │
│   ├── term.c            # raw mode, alt screen, terminal setup
│   ├── term.h
│   │
│   ├── input.c           # keyboard input parsing
│   ├── input.h
│   │
│   ├── buffer.c          # output buffer (single flush renderer)
│   ├── buffer.h
│   │
│   ├── render.c          # draw API (text, box, cursor, layout)
│   ├── render.h
│   │
│   ├── utf8.c            # UTF-8 width handling
│   ├── utf8.h
│   │
│   ├── theme.c           # theme loading + active theme
│   ├── theme.h
│   │
│   ├── config.c          # config parsing (JSON)
│   ├── config.h
│   │
│   ├── words.c           # wordbank loading from Monkeytype JSON
│   ├── words.h
│   │
│   ├── timer.c           # timing helpers
│   ├── timer.h
│   │
│   ├── stats.c           # WPM/accuracy/session stats
│   ├── stats.h
│   │
│   └── common.h          # shared types/macros
│
├── themes/
│   ├── catppuccin.theme
│   ├── gruvbox.theme
│   └── nord.theme
│
├── config/
│   └── config.lua
│
├── wordlists/
│   └── english.json
│
├── include/              # optional later
│
├── Makefile
└── README.md
```

---

# Header Files

---

## common.h

```c
#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define KB(x) ((x) * 1024)
#define ARRAY_LEN(x) (sizeof(x) / sizeof((x)[0]))

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;

#endif
```

---

## term.h

```c
#ifndef TERM_H
#define TERM_H

void term_init(void);
void term_shutdown(void);

void term_enter_alt_screen(void);
void term_leave_alt_screen(void);

void term_hide_cursor(void);
void term_show_cursor(void);

void term_get_size(int *rows, int *cols);

#endif
```

### Purpose

Handles:

* raw mode
* terminal cleanup
* alternate screen
* cursor visibility
* terminal size

---

## input.h

```c
#ifndef INPUT_H
#define INPUT_H

typedef enum {
    KEY_NONE,
    KEY_CHAR,
    KEY_ENTER,
    KEY_BACKSPACE,
    KEY_ESCAPE,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_CTRL_C,
} KeyType;

typedef struct {
    KeyType type;
    char ch;
} KeyEvent;

KeyEvent input_read(void);

#endif
```

### Purpose

Handles:

* reading keys
* parsing escape sequences
* future keybindings

---

## buffer.h

```c
#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>

#define BUFFER_CAP 65536

typedef struct {
    char data[BUFFER_CAP];
    size_t len;
} Buffer;

void buffer_clear(Buffer *b);
void buffer_append(Buffer *b, const char *text);
void buffer_printf(Buffer *b, const char *fmt, ...);
void buffer_flush(Buffer *b);

#endif
```

### Purpose

Single output buffer:

* build frame
* one terminal flush
* no scattered printf calls

---

## theme.h

```c
#ifndef THEME_H
#define THEME_H

#include <stdbool.h>

typedef struct {
    u8 r, g, b;
} RGB;

typedef struct {
    RGB fg;
    RGB bg;
    bool bold;
    bool dim;
    bool underline;
} Style;

typedef struct {
    Style correct;
    Style wrong;
    Style current;
    Style inactive;
    Style border;
    Style stats;
    Style title;
    Style timer;
} Theme;

extern Theme g_theme;

bool theme_load(const char *path);
void theme_load_default(void);

#endif
```

### Purpose

Handles:

* themes
* colors
* style flags
* theme switching

---

## render.h

```c
#ifndef RENDER_H
#define RENDER_H

#include "buffer.h"
#include "theme.h"

void render_clear(Buffer *b);
void render_move_cursor(Buffer *b, int row, int col);

void draw_text(
    Buffer *b,
    int row,
    int col,
    const char *text,
    Style style
);

void draw_box(
    Buffer *b,
    int row,
    int col,
    int width,
    int height,
    Style style
);

void render_frame(Buffer *b);

#endif
```

### Purpose

Your drawing engine:

* text
* boxes
* cursor
* layout rendering

---

## utf8.h

```c
#ifndef UTF8_H
#define UTF8_H

int utf8_display_width(const char *text);
int utf8_char_width(const char *text);

#endif
```

### Purpose

Handles:

* multilingual support
* correct alignment
* CJK + Arabic + accents
* rounded borders safely

---

## words.h

```c
#ifndef WORDS_H
#define WORDS_H

#include <stddef.h>

typedef struct {
    char** items;
    size_t count;
} WordList;

bool words_load_json(const char *path, WordList *list);
void words_free(WordList *list);

#endif
```

### Purpose

Handles:

* Monkeytype JSON parsing
* custom wordbanks
* word randomization later

---

## stats.h

```c
#ifndef STATS_H
#define STATS_H

typedef struct {
    int correct_chars;
    int wrong_chars;
    int total_chars;

    float wpm;
    float accuracy;
} Stats;

void stats_update(Stats *s);
void stats_reset(Stats *s);

#endif
```

### Purpose

Handles:

* WPM
* accuracy
* correctness
* live typing stats

---

## timer.h

```c
#ifndef TIMER_H
#define TIMER_H

void timer_start(void);
// tick ?
double timer_elapsed_seconds(void);

#endif
```

### Purpose

Handles:

* session timing
* countdown mode
* race timing later

---

## app.h

```c
#ifndef APP_H
#define APP_H

void app_init(void);
void app_update(void);
void app_render(void);
void app_shutdown(void);

#endif
```

### Purpose

High-level game loop logic:

* state transitions
* typing state
* restart logic
* mode switching

---

## config.h

```c
#ifndef CONFIG_H
#define CONFIG_H

bool config_load(const char *path);

#endif
```

### Purpose

Handles:

* config file parsing
* settings

---

# Strong Rule

## Never let modules leak responsibilities

Bad:

* renderer loading JSON
* theme parsing input
* app doing ANSI codes

Good:

* each module owns one job only

This keeps the project scalable.


