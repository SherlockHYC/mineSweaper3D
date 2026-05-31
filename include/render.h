#ifndef RENDER_H
#define RENDER_H

#include "game.h"

/* Call once at startup before any render_frame(). */
void render_init(void);

/* Call on exit — restores terminal to normal state. */
void render_cleanup(void);

/* Redraw the entire screen.
 * cur_x/y/z: current cursor position to highlight. */
void render_frame(const Game *g, int cur_x, int cur_y, int cur_z);

#endif
