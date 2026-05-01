#include "cmonkey.h"

#define ENG         "english_1k.json"
#define FOLDER_PATH "wordbanks/"
#define CURR_FILE   (FOLDER_PATH ENG)

#define THEME_PATH "config/catppuccin_mocha.theme"
#define CONF_PATH  "config/cmonkey.conf"


int main(void)
{
    cmonkey cm = {0};
    cmonkey_create(&cm, CURR_FILE, THEME_PATH, CONF_PATH);
    cmonkey_init_term(&cm);

    cmonkey_run(&cm);

    cmonkey_end_term();
    cmonkey_destroy(&cm);
    return 0;
}
