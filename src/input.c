#include <ncurses.h>
#include "input.h"

Command input_read(void) {
    int ch = getch();
    switch (ch) {
        case KEY_LEFT:  case 'a':           return (Command){CMD_MOVE_LEFT};
        case KEY_RIGHT: case 'd':           return (Command){CMD_MOVE_RIGHT};
        case KEY_UP:    case 'w':           return (Command){CMD_MOVE_UP};
        case KEY_DOWN:  case 's':           return (Command){CMD_MOVE_DOWN};
        case '<': case ',':                 return (Command){CMD_MOVE_IN};
        case '>': case '.':                 return (Command){CMD_MOVE_OUT};
        case ' ': case '\n': case '\r': case KEY_ENTER:
                                            return (Command){CMD_REVEAL};
        case 'f': case 'F':                 return (Command){CMD_FLAG};
        case 'x': case 'X':                 return (Command){CMD_DEBUG};
        case 'r': case 'R':                 return (Command){CMD_RESTART};
        case 'q': case 'Q':                 return (Command){CMD_QUIT};
        default:                            return (Command){CMD_NONE};
    }
}
