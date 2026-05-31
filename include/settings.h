#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>

#define BOARD_MIN 5
#define BOARD_MAX 8

typedef struct {
    int width;
    int height;
    int depth;
    int mines;
} Settings;

/* Clamp all fields to valid ranges. Call after any change. */
void settings_clamp(Settings *s);

/* Run the interactive settings screen (render_init must be called first).
 * Returns true if user pressed Enter to start, false if quit. */
bool settings_screen(Settings *s);

#endif
