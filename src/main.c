#include <ncurses.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>


static WINDOW* create_newwin(int height, int width, int starty, int startx);
static void    destroy_win(WINDOW* local_win);


static const char* text = "hello i am wasi ullah";
static size_t      i    = 0;


#define WORDS_PER_15SEC 30


#define HEIGHT 10
#define WIDTH  50
#define STARTY ((LINES - HEIGHT) / 2)
#define STARTX ((COLS - WIDTH) / 2)


int main(void)
{
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    refresh();      // refresh stdscr
    // create main window that will hold the text
    WINDOW* main_window = create_newwin(HEIGHT, WIDTH, STARTY, STARTX);

    size_t text_len = strlen(text);

    // Draw text once, outside loop
    mvwprintw(main_window, 2, 2, "%s", text);
    wrefresh(main_window);

    int c = 0;
    while (c != 'q' && i < text_len) 
    {
        c = getch();
        if (c == text[i]) 
        {
            mvwaddch(main_window, 2, 2 + i, (chtype)c | A_BOLD);
            mvwaddch(main_window, 2, 3 + i, (chtype)text[i + 1] | A_UNDERLINE);
            wrefresh(main_window);
            i++;
        }
    }

    destroy_win(main_window);
    endwin();
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

