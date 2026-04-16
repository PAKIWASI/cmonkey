#include "game.h"
#include "ui.h"


int main(void)
{
    UI   ui;
    Game game;

    ui_init(&ui);
    game_init(&game, "/home/wasi/Documents/projects/c/cmonkey/data/english.json");

    bool running = true;
    while (running) {

        // render
        if (game.state == GS_FINISHED) {
            ui_render_results(&ui, &game);
        } else {
            ui_render(&ui, &game);
        }

        // tick (update elapsed time)
        game_tick(&game);

        // input
        int ch = getch(); // returns ERR after timeout(100) ms if no key

        if (ch == ERR) { continue; } // just a timer tick, nothing typed


        // Global keys
        if (ch == 'q' || ch == 3 /* Ctrl-C */) {
            running = false;
            break;
        }

        if (ch == KEY_RESIZE) {
            ui_resize(&ui);
            continue;
        }

        if (game.state == GS_FINISHED) {
            if (ch == 'r') {
                game_new_round(&game);
            }
            continue;
        }

        // Forward to game logic
        game_input(&game, ch);
    }

    game_destroy(&game);
    ui_destroy(&ui);
    return 0;
}


/*
cmonkey/
├── CMakeLists.txt
├── data/
│   └── english.json
├── external/          ← your WCToolkit single-headers + jsmn (unchanged)
│   ├── arena_single.h
│   ├── gen_vector_single.h
│   ├── ...
│   └── jsmn.h
├── include/
│   ├── game.h         ← pure domain state, zero ncurses
│   ├── ui.h           ← ncurses window handles + render API
│   └── json_wordbank_loader.h
└── src/
    ├── main.c         ← thin loop: render → tick → input → dispatch
    ├── game.c         ← all typing logic, WPM, accuracy, timer
    ├── ui.c           ← all ncurses rendering
    ├── json_wordbank_loader.c
    └── wc_imp.c       ← single #define WC_IMPLEMENTATION translation unit
*/
