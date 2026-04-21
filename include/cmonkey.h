#ifndef CMONKEY_H
#define CMONKEY_H

#include "conf.h"

// cmonkey app state

typedef struct {
    cmonkey_theme theme;
    cmonkey_conf conf;

    bool resize;
    bool quit;
} cmonkey;




#endif // CMONKEY_H
