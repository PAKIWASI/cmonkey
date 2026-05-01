#ifndef INPUT_H
#define INPUT_H


typedef enum {
    INPUT_NONE = 0,
    INPUT_CHAR,           // printable character
    INPUT_BACKSPACE,      // delete last char
    INPUT_CTRL_BACKSPACE, // delete whole current word (Ctrl+W)
    INPUT_ESCAPE,         // quit / go back
    INPUT_TAB,            // restart test
    INPUT_CTRL_C,         // boom
} INPUT_TYPE;

typedef struct {
    INPUT_TYPE type;
    char       ch; // valid only when type == INPUT_CHAR
} cmonkey_input;

// Call once after raw mode is set — makes stdin non-blocking.
void input_init(void);

// Drain all bytes available this frame into out[0..max_inputs).
// Returns number of inputs written. Returns 0 when nothing is pending.
int input_poll(cmonkey_input* out, int max_inputs);

#endif // INPUT_H
