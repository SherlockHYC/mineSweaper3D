#ifndef BOARD_H
#define BOARD_H

#include <stdbool.h>
#include "cell.h"

/* ── Debug logging ───────────────────────────────────────────
 * Compile with -DDEBUG to enable.
 * All DBG() calls compile away to nothing in release builds.
 * ─────────────────────────────────────────────────────────── */
#ifdef DEBUG
#  include <stdio.h>
#  define DBG(fmt, ...) \
       fprintf(stderr, "[DBG %s:%d] " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#  define DBG(fmt, ...) ((void)0)
#endif

typedef struct {
    int          width, height, depth;  /* x, y, z dimensions */
    Cell        *cells;                 /* flat array [x*H*D + y*D + z] */
    int          total_mines;
    int          revealed_count;
    int          safe_count;            /* non-mine cells total */
    bool         debug_mode;            /* runtime flag: expose mine positions */
    unsigned int seed;                  /* RNG seed used — allows reproduction */
} Board;

/* ── Accessor macros ─────────────────────────────────────────
 * CELL_AT  : get a Cell reference by (x,y,z)
 * IN_BOUNDS: check whether coordinates are inside the board
 * ─────────────────────────────────────────────────────────── */
#define CELL_AT(b, x, y, z) \
    ((b)->cells[(x) * (b)->height * (b)->depth + (y) * (b)->depth + (z)])

#define IN_BOUNDS(b, x, y, z) \
    ((x) >= 0 && (x) < (b)->width  && \
     (y) >= 0 && (y) < (b)->height && \
     (z) >= 0 && (z) < (b)->depth)

/* ── Lifecycle ───────────────────────────────────────────── */
Board *board_create(int width, int height, int depth);
void   board_free(Board *b);

/* ── Setup (call in order) ───────────────────────────────── */
void   board_place_mines(Board *b, int count, unsigned int seed);
void   board_compute_adjacent(Board *b);   /* must run after place_mines */

/* ── Gameplay ────────────────────────────────────────────── */
bool   board_reveal(Board *b, int x, int y, int z);  /* true = hit mine */
void   board_flag  (Board *b, int x, int y, int z);  /* toggle flag */

/* ── Debug utilities ─────────────────────────────────────── */

/* Print all z-layers; respects debug_mode for mine visibility */
void board_debug_print(const Board *b);

/* Print a single z-layer */
void board_debug_print_layer(const Board *b, int z);

/* Manually set/clear one mine (updates counts) */
void board_debug_set_mine(Board *b, int x, int y, int z, bool val);

/* Replace all mines with a fixed pattern — useful for unit tests.
 * mines[][3] is an array of {x, y, z} triples, length = count. */
void board_debug_place_pattern(Board *b, const int mines[][3], int count);

/* Reveal every hidden cell (cheat / end-of-game reveal) */
void board_debug_reveal_all(Board *b);

/* ── Game-over helper ────────────────────────────────────── */

/* Reveal only mine cells that are still hidden (used on LOSE).
 * Does NOT update revealed_count — mines were never "found" by player. */
void board_expose_mines(Board *b);

#endif
