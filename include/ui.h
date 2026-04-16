#ifndef UI_H
#define UI_H

#include <ncurses.h>
#include "game.h"

// TODO: theming: file import based theming


// Holds all ncurses window handles.
typedef struct {
    WINDOW* header;   // top bar: title + time
    WINDOW* text;     // word display area
    WINDOW* status;   // bottom bar: WPM / accuracy / instructions
} UI;

// TODO: current color ? (if user typed wrong char, we want that to be red)



void ui_init(UI* ui);

void ui_destroy(UI* ui);

// call on KEY_RESIZE
void ui_resize(UI* ui);

void ui_render(UI* ui, const Game* g);

void ui_render_results(UI* ui, const Game* g);

#endif // UI_H
