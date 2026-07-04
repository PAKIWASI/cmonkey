#include "input.h"
#include "common_single.h"

#include <fcntl.h>
#include <unistd.h>



void input_init(void)
{
    // F_GETFL returns the file access mode and the file status flags
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags == -1) {
        WARN("fcntl F_GETFL failed");
        return;
    }
    // F_SETFL sets the file status flags to the value specified by arg
    if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1) {   // whatever the flags were, plus O_NONBLOCK
        WARN("fcntl F_SETFL O_NONBLOCK failed");
    }
}

static cmonkey_input process_char(unsigned char ch)
{
    cmonkey_input result = {ACTION_NONE, 0};
    
    switch (ch) {
        case 3:   // Ctrl-C
            result.action = ACTION_END;
            break;
        case 9:   // Tab
            result.action = ACTION_RESTART;
            break;
        case 127: // Backspace
        case 8:   // Backspace (alternate)
            result.action = ACTION_BACKSPACE;
            break;
        case 23:  // Ctrl-W
            result.action = ACTION_DEL_WORD;
            break;
        default:
            // Printable characters
            if (ch >= 32 && ch <= 126) {
                result.action = ACTION_CHAR;
                result.ch = (char)ch;
            }
            break;
    }
    
    return result;
}

u32 input_read_all(cmonkey_input* buffer)
{
    unsigned char raw[MAX_INPUTS * 2];
    ssize_t n = read(STDIN_FILENO, raw, sizeof(raw));
    
    if (n <= 0) {
        return 0;
    }
    
    u32 count = 0;
    for (ssize_t i = 0; i < n && count < MAX_INPUTS; i++) {
        cmonkey_input input = process_char(raw[i]);
        if (input.action != ACTION_NONE) {
            buffer[count++] = input;
        }
    }
    return count;
}



