#include "input.h"
#include "common_single.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>


void input_init(void)
{
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags == -1) {
        WARN("fcntl F_GETFL failed");
        return;
    }
    if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1) {
        WARN("fcntl F_SETFL O_NONBLOCK failed");
    }
}

int input_poll(cmonkey_input* out, int max_inputs)
{
    int count = 0;
    while (count < max_inputs) {
        u8      c;
        ssize_t n = read(STDIN_FILENO, &c, 1);

        if (n <= 0) {
            // EAGAIN / EWOULDBLOCK = nothing in the pipe this frame, not an error
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) { break; }
            // actual EOF or error — still break, caller doesn't need to know
            break;
        }

        cmonkey_input inp = {0};

        if      (c == 127)           { inp.type = INPUT_BACKSPACE;       }
        else if (c == 23)            { inp.type = INPUT_CTRL_BACKSPACE;  } // Ctrl+W
        else if (c == 27)            { inp.type = INPUT_ESCAPE;          } // ESC
        else if (c == 9)             { inp.type = INPUT_TAB;             } // Tab
        else if (c == 3)             { inp.type = INPUT_CTRL_C;          } // Ctrl+C
        else if (c >= 32 && c < 127) {
            inp.type = INPUT_CHAR;
            inp.ch   = (char)c;
        }
        // everything else (arrows, F-keys, multi-byte escapes): silently drop

        // only append if we actually mapped it to something
        if (inp.type != INPUT_NONE) {
            out[count++] = inp;
        }
    }
    return count;
}


