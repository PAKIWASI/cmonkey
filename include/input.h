#ifndef INPUT_H
#define INPUT_H

#include "common_single.h"

#define MAX_INPUTS 16

typedef enum {
    ACTION_NONE = 0,
    ACTION_CHAR,
    ACTION_BACKSPACE,
    ACTION_DEL_WORD,
    ACTION_RESTART,
    ACTION_END,
} ACTION;

typedef struct {
    ACTION action;
    char   ch;      // valid only for ACTION_CHAR
} cmonkey_input;

// Call once after raw mode is set — makes stdin non-blocking.
void input_init(void);

// Read ALL available input, fill the provided array, return count
// Returns number of valid inputs read (max MAX_INPUTS)
u32 input_read_all(cmonkey_input* buffer);

#endif // INPUT_H
