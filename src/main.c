#include "json_wordbank_loader.h"
#include <ncurses.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


static WINDOW* create_newwin(int height, int width, int starty, int startx);
static void    destroy_win(WINDOW* local_win);



#define HEIGHT 10
#define WIDTH  50
#define STARTY ((LINES - HEIGHT) / 2)
#define STARTX ((COLS - WIDTH) / 2)


static struct timespec start, now;
static bool            started = false;
static float           elapsed = 0.0F;


int main(void)
{
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    timeout(100);       // timeout for getch (it blocks)
    refresh();
    curs_set(0);    // 0: not visible, 1: visible, 2: very visible
    if (has_colors()) {
        start_color();
    }

    WINDOW* text_win   = create_newwin(HEIGHT - 3, WIDTH, STARTY, STARTX);
    WINDOW* status_win = create_newwin(3, WIDTH, STARTY - 3, STARTX);



    WordBank* wb = wordbank_create("../data/english.json");
    genVec* words = wordbank_random_words(wb, 5);


    // print text once outside loop
    size_t text_len = strlen(text);
    mvwprintw(text_win, 2, 2, "%s", text);
    wrefresh(text_win);

    int c = 0;
    while (1) {
        if (started) {
            timespec_get(&now, TIME_UTC);
            elapsed = (now.tv_sec - start.tv_sec) + ((now.tv_nsec - start.tv_nsec) / 1e9);
        }

        if (started && elapsed >= 15.0) {
            break;
        }
        if (i >= text_len) {
            break;
        }

        if (started) {
            mvwprintw(status_win, 1, 2, "Time: %.0f ", 15.0 - elapsed);
        } else {
            mvwprintw(status_win, 1, 2, "Time: 15   ");
        }
        wrefresh(status_win);

        c = getch();
        if (c == ERR) {
            continue;
        }
        // TODO: not working
        if (c == KEY_ENTER) {
            break;
        }

        if (c == text[i]) {
            if (!started) {
                timespec_get(&start, TIME_UTC);
                started = true;
            }
            mvwaddch(text_win, 2, 2 + i, (chtype)c | A_BOLD);
            mvwaddch(text_win, 2, 2 + i + 1, (chtype)text[i + 1] | A_UNDERLINE);
            wrefresh(text_win);
            i++;
        }
    }

    destroy_win(text_win);
    destroy_win(status_win);
    endwin();

    if (started) {
        float duration = elapsed < 15.0 ? elapsed : 15.0F;
        float wpm      = ((float)i / 5.0F) / (duration / 60.0F);
        // TODO: i doesnot count words
        printf("Typed: %zu chars | WPM: %.1f\n", i, wpm);
    } else {
        printf("Test not started.\n");
    }

    wordbank_destroy(wb);
    return 0;
}


WINDOW* create_newwin(int height, int width, int starty, int startx)
{
    WINDOW* local_win = newwin(height, width, starty, startx);
    box(local_win, 0, 0);
    wrefresh(local_win);
    return local_win;
}

void destroy_win(WINDOW* local_win)
{
    wborder(local_win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    wrefresh(local_win);
    delwin(local_win);
}



/* TODO:

    wnoutrefresh(header);  // stage the diff
    wnoutrefresh(text);
    wnoutrefresh(status);
    doupdate();            // single terminal write — avoids flicker
    

    project structure: 

src/
  main.c          ← entry point, init/cleanup, main loop
  app.h/.c        ← top-level app state struct
  ui.h/.c         ← all rendering logic
  input.h/.c      ← key handling, input dispatch
  game.h/.c       ← your domain logic (typing test, etc.) — no ncurses here
    

    main loop example:

typedef enum { STATE_TYPING, STATE_RESULTS, STATE_QUIT } AppState;

int main(void) {
    // init ncurses ...

    App app;
    app_init(&app);

    while (app.state != STATE_QUIT) {
        // 1. render current state
        ui_render(&app);

        // 2. block for one keypress
        int ch = getch();

        // 3. dispatch to input handler
        input_handle(&app, ch);

        // 4. update domain logic (timers, scoring, etc.)
        app_update(&app);
    }

    app_destroy(&app);
    endwin();
    return 0;
}


handle resize:

if (ch == KEY_RESIZE) {
    // ncurses updates LINES and COLS globals automatically
    endwin();
    refresh(); // forces ncurses to re-probe terminal size
    ui_resize(&app); // re-create or move your WINDOWs
}


ui render pattern:

void ui_render(App* app) {
    werase(app->text_win);

    // draw words, highlight current position, etc.
    for (int i = 0; i < app->word_count; i++) {
        if (i < app->cursor)      wattron(app->text_win, COLOR_PAIR(1)); // correct
        else if (i == app->cursor) wattron(app->text_win, A_BOLD);
        mvwprintw(app->text_win, row, col, "%s ", app->words[i]);
        wattroff(app->text_win, A_BOLD | COLOR_PAIR(1));
    }

    wnoutrefresh(app->text_win);
    doupdate();
}

*/
