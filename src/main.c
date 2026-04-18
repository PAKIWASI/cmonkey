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

    }


    game_destroy(&game);
    ui_destroy(&ui);

    return 0;
}


