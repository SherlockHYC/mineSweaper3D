#include <stdlib.h>
#include <time.h>
#include "game.h"

/* ── Internal helpers ────────────────────────────────────── */

/* If (x,y,z) is a mine, relocate it to a randomly chosen non-mine cell.
 * Moving to the first cell found (always index 0) was the bug:
 * the mine predictably ended up at (0,0,0) every time. */
static void first_click_safety(Game *g, int x, int y, int z) {
    Board *b = g->board;
    if (!CELL_AT(b, x, y, z).has_mine) return;

    int  total      = b->width * b->height * b->depth;
    int *candidates = malloc((size_t)total * sizeof(int));
    if (!candidates) return;

    int ncand = 0;
    for (int fx = 0; fx < b->width;  fx++)
    for (int fy = 0; fy < b->height; fy++)
    for (int fz = 0; fz < b->depth;  fz++) {
        if (fx == x && fy == y && fz == z)    continue;
        if (CELL_AT(b, fx, fy, fz).has_mine)  continue;
        candidates[ncand++] = fx * b->height * b->depth
                            + fy * b->depth
                            + fz;
    }

    if (ncand == 0) { free(candidates); return; }

    int flat = candidates[rand() % ncand];
    int fx   = flat / (b->height * b->depth);
    int fy   = (flat / b->depth) % b->height;
    int fz   = flat % b->depth;
    free(candidates);

    board_debug_set_mine(b, x,  y,  z,  false);
    board_debug_set_mine(b, fx, fy, fz, true);
    board_compute_adjacent(b);
    DBG("first-click safety: mine moved (%d,%d,%d)->(%d,%d,%d)",
        x, y, z, fx, fy, fz);
}

/* ── Lifecycle ───────────────────────────────────────────── */

Game *game_init(int width, int height, int depth, int mines, unsigned int seed) {
    Game *g = malloc(sizeof(Game));
    if (!g) return NULL;

    g->board = board_create(width, height, depth);
    if (!g->board) { free(g); return NULL; }

    if (seed == 0) seed = (unsigned int)time(NULL);

    board_place_mines(g->board, mines, seed);
    board_compute_adjacent(g->board);

    g->state        = GAME_WAITING;
    g->start_time   = 0;
    g->elapsed_sec  = 0;
    g->flags_placed = 0;

    DBG("game_init %dx%dx%d mines=%d seed=%u", width, height, depth, mines, seed);
    return g;
}

void game_free(Game *g) {
    if (!g) return;
    board_free(g->board);
    free(g);
}

/* ── Gameplay ────────────────────────────────────────────── */

GameState game_reveal(Game *g, int x, int y, int z) {
    if (g->state == GAME_WIN || g->state == GAME_LOSE) return g->state;
    if (!IN_BOUNDS(g->board, x, y, z))                 return g->state;

    if (g->state == GAME_WAITING) {
        g->state      = GAME_PLAYING;
        g->start_time = time(NULL);
        first_click_safety(g, x, y, z);
        DBG("first move: clock started, seed=%u", g->board->seed);
    }

    bool hit = board_reveal(g->board, x, y, z);

    if (hit) {
        g->state = GAME_LOSE;
        g->elapsed_sec = (int)(time(NULL) - g->start_time);
        board_expose_mines(g->board);   /* show all remaining mines */
        DBG("LOSE at (%d,%d,%d)", x, y, z);
        return GAME_LOSE;
    }

    if (g->board->revealed_count == g->board->safe_count) {
        g->state = GAME_WIN;
        g->elapsed_sec = (int)(time(NULL) - g->start_time);
        DBG("WIN: revealed=%d/%d in %ds",
            g->board->revealed_count, g->board->safe_count, g->elapsed_sec);
        return GAME_WIN;
    }

    return GAME_PLAYING;
}

GameState game_flag(Game *g, int x, int y, int z) {
    if (g->state == GAME_WIN || g->state == GAME_LOSE) return g->state;
    if (!IN_BOUNDS(g->board, x, y, z))                 return g->state;

    Cell *c = &CELL_AT(g->board, x, y, z);
    if (c->state == CELL_REVEALED) return g->state;

    bool was_flagged = (c->state == CELL_FLAGGED);
    board_flag(g->board, x, y, z);

    if (was_flagged) g->flags_placed--;
    else             g->flags_placed++;

    DBG("flag (%d,%d,%d): flags_placed=%d", x, y, z, g->flags_placed);
    return g->state;
}

/* ── Time ────────────────────────────────────────────────── */

void game_tick(Game *g) {
    if (g->state != GAME_PLAYING) return;
    g->elapsed_sec = (int)(time(NULL) - g->start_time);
}

int game_mines_remaining(const Game *g) {
    return g->board->total_mines - g->flags_placed;
}
