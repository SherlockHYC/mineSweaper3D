#ifndef GAME_H
#define GAME_H

#include <time.h>
#include "board.h"

typedef enum {
    GAME_WAITING,   /* board ready, no moves yet (first-click safety active) */
    GAME_PLAYING,
    GAME_WIN,
    GAME_LOSE
} GameState;

typedef struct {
    Board     *board;
    GameState  state;
    time_t     start_time;
    int        elapsed_sec;
    int        flags_placed;
} Game;

/* seed=0 → use time(NULL) as seed */
Game *game_init(int width, int height, int depth, int mines, unsigned int seed);
void  game_free(Game *g);

/* Both return the new GameState after the action. */
GameState game_reveal(Game *g, int x, int y, int z);
GameState game_flag  (Game *g, int x, int y, int z);

/* Call once per frame/tick to keep elapsed_sec current. */
void game_tick(Game *g);

/* total_mines - flags_placed (can go negative if over-flagged) */
int  game_mines_remaining(const Game *g);

#endif
