#include "cell.h"

void cell_init(Cell *c) {
    c->state    = CELL_HIDDEN;
    c->has_mine = false;
    c->adjacent = 0;
}
