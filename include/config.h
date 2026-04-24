#ifndef CMONKEY_CONFIG
#define CMONKEY_CONFIG


#define COLOR_ESC_MAX 32      // enough for "\033[38;2;255;255;255m"


// TODO: do i need null terminator?
typedef struct {
    // Core colours
    char main_fg    [COLOR_ESC_MAX];
    char main_bg    [COLOR_ESC_MAX];

    // Semantic colour roles
    char border     [COLOR_ESC_MAX];
    char cursor     [COLOR_ESC_MAX];
    char text_fg    [COLOR_ESC_MAX];
    char text_bg    [COLOR_ESC_MAX];
    char text_dim   [COLOR_ESC_MAX];
    char correct    [COLOR_ESC_MAX];
    char incorrect  [COLOR_ESC_MAX];

    // Global reset = "\033[0m" + main_fg + main_bg
    char reset      [(COLOR_ESC_MAX * 2) + 8];
} cmonkey_theme;


typedef enum {
    BORDER_SHARP    = 0,
    BORDER_ROUND,
    BORDER_BOLD,
    BORDER_DOUBLE,
} BORDER_STYLE;


// tl, tr, bl, br, vertical, horizontal
static const char BORDER_CHARS[4][6][8] = {
    {},
    {"╭", "╮", "╰", "╯", "│", "─"},
    {},
    {},
};

typedef enum {
    CURSOR_BAR      = 0,
    CURSOR_BLOCK,
    CURSOR_UNDERLINE,
} CURSOR_STYLE;


typedef struct {
    /* for later
    u32 trail_len;
    float trail_decay_ms;
    */
    BORDER_STYLE border_style;
    CURSOR_STYLE cursor_style;
} cmonkey_conf;


#define DEFAULT_BORDER_STYLE BORDER_ROUND
#define DEFAULT_CURSOR_STYLE CURSOR_BAR



// load theme file and convert values to ansi escape seq
void theme_load(cmonkey_theme* t, const char* filepath);

void config_load(cmonkey_conf* c, const char* filepath);



#endif // CMONKEY_CONFIG
