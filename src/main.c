#include "game.h"
#include "ui.h"


#define FILE_PATH_ENG "/home/wasi/Documents/projects/c/cmonkey/data/english.json"
#define FILE_PATH_ENG1K "/home/wasi/Documents/projects/c/cmonkey/data/english_1k.json"
#define FILE_PATH_ENG5K "/home/wasi/Documents/projects/c/cmonkey/data/english_10k.json"
#define FILE_PATH_ENG10K "/home/wasi/Documents/projects/c/cmonkey/data/english_5k.json"
#define FILE_PATH_ENG25K "/home/wasi/Documents/projects/c/cmonkey/data/english_25k.json"
#define FILE_PATH_ENG450K "/home/wasi/Documents/projects/c/cmonkey/data/english_450k.json"
#define FILE_PATH_ENG_MISSPELLED "/home/wasi/Documents/projects/c/cmonkey/data/english_commonly_misspelled.json"
#define FILE_PATH_ARABIC "/home/wasi/Documents/projects/c/cmonkey/data/arabic.json"
#define FILE_PATH_C "/home/wasi/Documents/projects/c/cmonkey/data/code_c.json"

#define CURR_FILE FILE_PATH_ENG450K


int main(void)
{
    UI   ui;
    Game game;

    ui_init(&ui);
    game_init(&game, CURR_FILE);

    // clear();
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

    u64 used = game.bank->arena->idx;
    u64 cap = game.bank->arena->size;
    u64 words = game.bank->words->size;

    game_destroy(&game);
    ui_destroy(&ui);

    LOG("arena used: %lu", used);
    LOG("arena cap: %lu", cap);
    LOG("genVec size: %lu", words);
    return 0;
}


