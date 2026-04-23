#ifndef CMONKEY_CONF_H
#define CMONKEY_CONF_H

#include "theme.h"



// runtime cursor behavior — appearance lives in cmonkey_theme
typedef struct {
    u8    cursor_trail_len;       // 0 = disabled, max 4
    float cursor_trail_decay_ms;  // how fast trail fades to bg
} cmonkey_conf;


// defaults — used when no conf file found
static inline cmonkey_conf conf_default(void)
{
    return (cmonkey_conf){
        .cursor_trail_len      = 0,
        .cursor_trail_decay_ms = 120.0f,
    };
}


bool cmonkey_import_conf(cmonkey_conf* conf, const char* confpath);

bool cmonkey_import_theme(cmonkey_theme* t, const char* themepath);


#endif // CMONKEY_CONF_H
