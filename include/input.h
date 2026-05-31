#ifndef INPUT_H
#define INPUT_H

typedef enum {
    CMD_NONE,
    CMD_MOVE_LEFT,   /* x--  : a / LEFT  */
    CMD_MOVE_RIGHT,  /* x++  : d / RIGHT */
    CMD_MOVE_UP,     /* y--  : w / UP    */
    CMD_MOVE_DOWN,   /* y++  : s / DOWN  */
    CMD_MOVE_IN,     /* z--  : < / ,     */
    CMD_MOVE_OUT,    /* z++  : > / .     */
    CMD_REVEAL,      /* reveal cursor cell : SPACE / ENTER */
    CMD_FLAG,        /* flag / unflag      : f             */
    CMD_DEBUG,       /* toggle debug mode  : x             */
    CMD_RESTART,     /* return to settings : r             */
    CMD_QUIT         /* quit               : q             */
} CmdType;

typedef struct {
    CmdType type;
} Command;

/* Blocking: waits for one keypress. render_init() must be called first. */
Command input_read(void);

#endif
