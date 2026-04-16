#include "ui.h"
#include "json_wordbank_loader.h"
#include <ncurses.h>
#include <string.h>


// color pair IDs
#define CP_NORMAL    1
#define CP_CORRECT   2   // green
#define CP_WRONG     3   // red
#define CP_CURSOR    4   // bold white / underline
#define CP_DIM       5   // grey-ish untyped text
#define CP_HEADER    6   // header bar

// layout helpers

// Heights are fixed; widths stretch full terminal.
#define HEADER_H  3
#define STATUS_H  3

// text window fills what's in between
#define TEXTBOX_H (LINES / 2)
#define TEXTBOX_W (COLS / 2)
#define TEXTBOX_Y (((LINES - HEADER_H) / 2) - (TEXTBOX_H / 2))
#define TEXTBOX_X (TEXTBOX_W - (TEXTBOX_W / 2))


// window creation

static WINDOW* make_win(int h, int w, int y, int x)
{
    WINDOW* win = newwin(h, w, y, x);
    box(win, 0, 0);
    return win;
}

static void rebuild_windows(UI* ui)
{
    if (ui->header) { delwin(ui->header); }
    if (ui->text)   { delwin(ui->text); }
    if (ui->status) { delwin(ui->status); }

    ui->header = make_win(HEADER_H, COLS, 0,          0);
    ui->text   = make_win(TEXTBOX_H, TEXTBOX_W, TEXTBOX_Y, TEXTBOX_X);
    ui->status = make_win(STATUS_H, COLS, LINES - STATUS_H, 0);
}


// init / destroy

void ui_init(UI* ui)
{
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(100);  // getch() times out every 100 ms so game_tick() fires
    refresh();

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(CP_NORMAL,  COLOR_WHITE,  -1);
        init_pair(CP_CORRECT, COLOR_GREEN,  -1);
        init_pair(CP_WRONG,   COLOR_RED,    -1);
        init_pair(CP_CURSOR,  COLOR_WHITE,  -1);
        init_pair(CP_DIM,     COLOR_WHITE,  -1);  // will use A_DIM attribute
        init_pair(CP_HEADER,  COLOR_CYAN,   -1);
    }

    ui->header = NULL;
    ui->text   = NULL;
    ui->status = NULL;
    rebuild_windows(ui);
}

void ui_destroy(UI* ui)
{
    delwin(ui->header);
    delwin(ui->text);
    delwin(ui->status);
    endwin();
}

void ui_resize(UI* ui)
{
    endwin();
    refresh();
    clear();
    rebuild_windows(ui);
}


// render helpers

// Draw a box + centred title on a window.
static void draw_titled_box(WINDOW* win, const char* title)
{
    werase(win);
    box(win, 0, 0);
    int w = getmaxx(win);
    int tx = (w - (int)strlen(title)) / 2;
    if (tx < 1) { tx = 1; }
    mvwprintw(win, 0, tx, " %s ", title);
}


// ui_render (typing screen)
//
// Layout inside text_win:
//   Row 1: words printed left-to-right, space-separated
//          coloured by per-word result + current char highlighted
//
// We wrap at COLS-4 (leaving 2 chars border each side).
void ui_render(UI* ui, const Game* g)
{
    int inner_w = getmaxx(ui->text) - 4;  // usable columns inside border+padding

    // header
    draw_titled_box(ui->header, "cmonkey");
    wattron(ui->header, COLOR_PAIR(CP_HEADER) | A_BOLD);

    if (g->state == GS_WAITING) {
        mvwprintw(ui->header, 1, 2, "Time: %.0f   Start typing!", GAME_DURATION_S);
    } else {
        mvwprintw(ui->header, 1, 2, "Time: %4.1f", game_time_left(g));
    }
    wattroff(ui->header, COLOR_PAIR(CP_HEADER) | A_BOLD);

    // text window
    werase(ui->text);
    box(ui->text, 0, 0);

    int row = 1, col = 2;  // start inside border

    for (u32 wi = 0; wi < g->word_count; wi++) 
    {
        u32         idx  = *(u32*)genVec_get_ptr(g->indices, wi);
        const char* word = wordbank_word_at(g->bank, idx);
        int         wlen = (int)strlen(word);

        // Wrap to next row if needed (+1 for the space separator)
        if (col + wlen + 1 > inner_w + 2 && col != 2) {
            row++;
            col = 2;
            if (row >= getmaxy(ui->text) - 1) { break; } // out of window space
        }

        if (wi < g->word_pos) {
            // Already typed
            int attr = (g->results[wi] == WR_CORRECT)
                       ? (COLOR_PAIR(CP_CORRECT) | A_BOLD)
                       : (COLOR_PAIR(CP_WRONG)   | A_BOLD);
            wattron(ui->text, attr);
            mvwprintw(ui->text, row, col, "%s", word);
            wattroff(ui->text, attr);

        } else if (wi == g->word_pos) {
            // Current word: colour each char individually
            for (int ci = 0; ci < wlen; ci++) {
                if (ci < (int)g->char_pos) {
                    // typed correctly so far
                    wattron(ui->text, COLOR_PAIR(CP_CORRECT));
                    mvwaddch(ui->text, row, col + ci, (chtype)word[ci]);
                    wattroff(ui->text, COLOR_PAIR(CP_CORRECT));
                } else if (ci == (int)g->char_pos) {
                    // cursor position
                    wattron(ui->text, A_UNDERLINE | A_BOLD);
                    mvwaddch(ui->text, row, col + ci, (chtype)word[ci]);
                    wattroff(ui->text, A_UNDERLINE | A_BOLD);
                } else {
                    // not yet typed
                    wattron(ui->text, A_DIM);
                    mvwaddch(ui->text, row, col + ci, (chtype)word[ci]);
                    wattroff(ui->text, A_DIM);
                }
            }

        } else {
            // Future words
            wattron(ui->text, A_DIM);
            mvwprintw(ui->text, row, col, "%s", word);
            wattroff(ui->text, A_DIM);
        }

        col += wlen + 1; // +1 for space
    }

    // status bar
    draw_titled_box(ui->status, "");
    mvwprintw(ui->status, 1, 2,
              "WPM: %5.1f  |  Acc: %5.1f%%  |  [Space] next word  [Ctrl-C] quit",
              game_wpm(g), game_accuracy(g));

    // flush
    wnoutrefresh(ui->header);
    wnoutrefresh(ui->text);
    wnoutrefresh(ui->status);
    doupdate();
}


// ui_render_results

void ui_render_results(UI* ui, const Game* g)
{
    werase(stdscr);
    wnoutrefresh(stdscr);

    draw_titled_box(ui->header, "cmonkey — results");
    draw_titled_box(ui->text,   "");
    draw_titled_box(ui->status, "");

    // results in text window
    mvwprintw(ui->text, 2, 4, "WPM      : %.1f", game_wpm(g));
    mvwprintw(ui->text, 3, 4, "Accuracy : %.1f%%", game_accuracy(g));
    mvwprintw(ui->text, 4, 4, "Time     : %.1f s", g->elapsed_s);
    mvwprintw(ui->text, 5, 4, "Words    : %u / %u",
              g->word_pos, g->word_count);

    mvwprintw(ui->status, 1, 2,
              "[r] new round   [q] quit");

    wnoutrefresh(ui->header);
    wnoutrefresh(ui->text);
    wnoutrefresh(ui->status);
    doupdate();
}

