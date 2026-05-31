#include <stdlib.h>
#include <stdio.h>
#include "game.h"
#include "render.h"
#include "input.h"
#include "settings.h"

/* ── Cursor helpers ──────────────────────────────────────── */

static void cursor_clamp(const Board *b, int *x, int *y, int *z) {
    if (*x < 0)          *x = 0;
    if (*x >= b->width)  *x = b->width  - 1;
    if (*y < 0)          *y = 0;
    if (*y >= b->height) *y = b->height - 1;
    if (*z < 0)          *z = 0;
    if (*z >= b->depth)  *z = b->depth  - 1;
}

/* ── Game loop ───────────────────────────────────────────── */

/* Returns true  → go back to settings (user pressed r)
 * Returns false → quit (user pressed q) */
static bool run(Game *g) {
    int cx = 0, cy = 0, cz = 0;

    for (;;) {
        game_tick(g);
        render_frame(g, cx, cy, cz);

        Command cmd = input_read();
        bool active = (g->state == GAME_WAITING || g->state == GAME_PLAYING);

        switch (cmd.type) {
            /* Movement always available — useful for post-game inspection */
            case CMD_MOVE_LEFT:  cx--; break;
            case CMD_MOVE_RIGHT: cx++; break;
            case CMD_MOVE_UP:    cy--; break;
            case CMD_MOVE_DOWN:  cy++; break;
            case CMD_MOVE_IN:    cz--; break;
            case CMD_MOVE_OUT:   cz++; break;

            /* Digging and flagging disabled after win/lose */
            case CMD_REVEAL:
                if (active) game_reveal(g, cx, cy, cz);
                break;
            case CMD_FLAG:
                if (active) game_flag(g, cx, cy, cz);
                break;

            /* Always available */
            case CMD_DEBUG:
                g->board->debug_mode = !g->board->debug_mode;
                break;
            case CMD_RESTART:
                return true;
            case CMD_QUIT:
                return false;
            default:
                break;
        }

        cursor_clamp(g->board, &cx, &cy, &cz);
    }
}

/* ── Entry point ─────────────────────────────────────────── */

int main(void) {
    /* Default settings: smallest valid board with a playable mine count */
    Settings s = {
        .width  = BOARD_MIN,
        .height = BOARD_MIN,
        .depth  = BOARD_MIN,
        .mines  = 20
    };
    settings_clamp(&s);

    render_init();

    bool keep_going = true;
    while (keep_going) {
        /* Settings screen — returns false if user quits from here */
        if (!settings_screen(&s)) break;

        Game *g = game_init(s.width, s.height, s.depth, s.mines, 0);
        if (!g) break;

        keep_going = run(g);
        game_free(g);
    }

    render_cleanup();
    return 0;
}
