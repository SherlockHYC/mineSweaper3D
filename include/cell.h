#ifndef CELL_H
#define CELL_H

#include <stdbool.h>

typedef enum {
    CELL_HIDDEN,
    CELL_REVEALED,
    CELL_FLAGGED
} CellState;

typedef struct {
    CellState state;
    bool      has_mine;
    int       adjacent;   /* 0–26; -1 if cell itself is a mine */
} Cell;

void cell_init(Cell *c);

#endif
