#include "config.h"


int main(void)
{
    cmonkey_theme t = {0};
    theme_load(&t, "config/dracula.theme");
    cmonkey_conf c = {0};
    config_load(&c, "config/cmonkey.conf");

    return 0;
}
