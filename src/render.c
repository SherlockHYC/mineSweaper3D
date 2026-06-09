#include <ncurses.h>
#include "render.h"
#include "board.h"
#include "game.h"

/* ── Color pair IDs ──────────────────────────────────────── */
enum {
    CP_DEFAULT = 0,
    CP_NUM_1,    /* blue           */
    CP_NUM_2,    /* green          */
    CP_NUM_3,    /* red            */
    CP_NUM_4,    /* blue bold      */
    CP_NUM_5,    /* red bold       */
    CP_NUM_6,    /* cyan           */
    CP_NUM_7,    /* magenta        */
    CP_NUM_8,    /* white          */
    CP_HIGH,     /* 9-26: white bold */
    CP_MINE,     /* bright red     */
    CP_FLAG,     /* yellow         */
    CP_WIN,      /* green bold     */
    CP_LOSE,     /* red bold       */
    CP_DEBUG_HDR /* red dim        */
};

/* ── Lifecycle ───────────────────────────────────────────── */

void render_init(void) {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(CP_NUM_1,    COLOR_BLUE,    -1);
        init_pair(CP_NUM_2,    COLOR_GREEN,   -1);
        init_pair(CP_NUM_3,    COLOR_RED,     -1);
        init_pair(CP_NUM_4,    COLOR_BLUE,    -1);
        init_pair(CP_NUM_5,    COLOR_RED,     -1);
        init_pair(CP_NUM_6,    COLOR_CYAN,    -1);
        init_pair(CP_NUM_7,    COLOR_MAGENTA, -1);
        init_pair(CP_NUM_8,    COLOR_WHITE,   -1);
        init_pair(CP_HIGH,     COLOR_WHITE,   -1);
        init_pair(CP_MINE,     COLOR_RED,     -1);
        init_pair(CP_FLAG,     COLOR_YELLOW,  -1);
        init_pair(CP_WIN,      COLOR_GREEN,   -1);
        init_pair(CP_LOSE,     COLOR_RED,     -1);
        init_pair(CP_DEBUG_HDR, COLOR_RED,    -1);
    }
}

void render_cleanup(void) {
    endwin();
}

/* ── Cell rendering helpers ──────────────────────────────── */

/* Character for adjacent count 0-26.
 * 0:'.', 1-9:'1'-'9', 10-26:'A'-'Q' */
static char adj_char(int adj) {
    if (adj == 0)        return '.';
    if (adj <= 9)        return (char)('0' + adj);
    if (adj <= 26)       return (char)('A' + adj - 10);
    return '?';
}

static char cell_ch(const Cell *c, bool show_mines) {
    if (show_mines && c->has_mine)
        return c->state == CELL_REVEALED ? 'X' :
               c->state == CELL_FLAGGED  ? 'F' : '*';
    switch (c->state) {
        case CELL_HIDDEN:   return '#';
        case CELL_FLAGGED:  return 'F';
        case CELL_REVEALED:
            if (c->has_mine) return 'X';
            return adj_char(c->adjacent);
    }
    return '?';
}

static int cell_pair(const Cell *c, bool show_mines) {
    if (c->state == CELL_FLAGGED)             return CP_FLAG;
    if (c->has_mine && c->state == CELL_REVEALED) return CP_MINE;
    if (show_mines && c->has_mine)            return CP_MINE;
    if (c->state != CELL_REVEALED)            return CP_DEFAULT;

    /* Revealed safe cell: color by adjacent count */
    static const int pairs[] = {
        CP_DEFAULT,           /* 0  */
        CP_NUM_1, CP_NUM_2, CP_NUM_3, CP_NUM_4,
        CP_NUM_5, CP_NUM_6, CP_NUM_7, CP_NUM_8
    };
    if (c->adjacent <= 8) return pairs[c->adjacent];
    return CP_HIGH;
}

static int cell_attrs(const Cell *c, bool is_cursor) {
    int a = 0;
    /* Bold for non-zero revealed numbers */
    if (c->state == CELL_REVEALED && !c->has_mine && c->adjacent > 0)
        a |= A_BOLD;
    if (is_cursor)
        a |= A_REVERSE;
    return a;
}

static void draw_cell(const Cell *c, bool is_cursor, bool show_mines) {
    int attr = COLOR_PAIR(cell_pair(c, show_mines)) | cell_attrs(c, is_cursor);
    attron(attr);
    printw(" %c", cell_ch(c, show_mines));
    attroff(attr);
}

/* ── Isometric cube overview ─────────────────────────────────
 * Renders the board as a Rubik's-cube-style isometric box.
 * Only three surface faces are drawn (everything else is hidden):
 *
 *   TOP   face  z = D-1           all (bx, by)
 *   FRONT face  y = H-1, bz<D-1  all (bx, bz)
 *   RIGHT face  x = W-1, by<H-1, bz<D-1
 *
 * Unified isometric formula (z-step = 2 screen-rows per layer):
 *   scol = base_col + (bx − by + H−1) × 2
 *   srow = base_row + (bx + by) + (D−1−bz) × 2
 *
 * These three faces provably share no screen cell, so painter's order
 * matters only for the top face (drawn last) over any edge coincidence.
 *
 * Color:
 *   bz == cur_z  → A_BOLD   (current layer strip / full top face)
 *   bz != cur_z  → A_DIM
 *   cursor cell  → A_REVERSE overlay
 * ─────────────────────────────────────────────────────────── */

/* helper macro — undefed at end of function */
#define ISO_COL(bx,by)    (base_col + ((bx)-(by)+b->height-1)*2)
#define ISO_ROW(bx,by,bz) (base_row + ((bx)+(by)) + (b->depth-1-(bz))*2)

static void draw_iso_cell(const Cell *c, int scol, int srow,
                           bool cur_layer, bool is_cursor,
                           bool show_mines) {
    int attr = COLOR_PAIR(cell_pair(c, show_mines));
    if (cur_layer)  attr |= A_BOLD;
    else            attr |= A_DIM;
    if (is_cursor)  attr |= A_REVERSE;
    attron(attr);
    mvprintw(srow, scol, "%c ", cell_ch(c, show_mines));
    attroff(attr);
}

static void render_iso_section(const Board *b,
                                int cur_x, int cur_y, int cur_z,
                                int base_row, int base_col,
                                bool show_mines) {
    int W = b->width, H = b->height, D = b->depth;

    /* ── FRONT face: y = H-1, bz = 0 … D-2 ── */
    for (int bz = 0; bz < D-1; bz++)
    for (int bx = 0; bx < W;   bx++) {
        draw_iso_cell(&CELL_AT(b, bx, H-1, bz),
                      ISO_COL(bx, H-1), ISO_ROW(bx, H-1, bz),
                      bz == cur_z,
                      bx == cur_x && H-1 == cur_y && bz == cur_z,
                      show_mines);
    }

    /* ── RIGHT face: x = W-1, by = 0 … H-2, bz = 0 … D-2 ── */
    for (int bz = 0; bz < D-1; bz++)
    for (int by = 0; by < H-1; by++) {
        draw_iso_cell(&CELL_AT(b, W-1, by, bz),
                      ISO_COL(W-1, by), ISO_ROW(W-1, by, bz),
                      bz == cur_z,
                      W-1 == cur_x && by == cur_y && bz == cur_z,
                      show_mines);
    }

    /* ── TOP face: z = D-1, drawn last so it sits on top ── */
    for (int by = H-1; by >= 0; by--)
    for (int bx = 0;  bx < W;  bx++) {
        draw_iso_cell(&CELL_AT(b, bx, by, D-1),
                      ISO_COL(bx, by), ISO_ROW(bx, by, D-1),
                      D-1 == cur_z,
                      bx == cur_x && by == cur_y && D-1 == cur_z,
                      show_mines);
    }
}

#undef ISO_COL
#undef ISO_ROW

/* Screen rows consumed by the iso section.
 * Lowest pixel: front-face bottom = (W-1+H-1) + (D-1)*2 → +1 for height. */
static int iso_section_height(const Board *b) {
    return (b->width + b->height - 2) + (b->depth - 1) * 2 + 1;
}

/* ── render_frame ────────────────────────────────────────── */

void render_frame(const Game *g, int cur_x, int cur_y, int cur_z) {
    const Board *b    = g->board;
    bool         show = b->debug_mode;
    int          row  = 0;

    clear();

    /* ── HUD ── */
    attron(A_BOLD);
    mvprintw(row, 0, "3D Minesweeper [%dx%dx%d]  Mines: %-3d  Time: %ds",
             b->width, b->height, b->depth,
             game_mines_remaining(g), g->elapsed_sec);
    attroff(A_BOLD);

    if (show) {
        attron(COLOR_PAIR(CP_DEBUG_HDR));
        printw("  [DEBUG]");
        attroff(COLOR_PAIR(CP_DEBUG_HDR));
    }
    row++;

    /* State banner */
    if (g->state == GAME_WIN) {
        attron(COLOR_PAIR(CP_WIN) | A_BOLD);
        mvprintw(row, 0, "  *** YOU WIN!  Time: %ds ***", g->elapsed_sec);
        attroff(COLOR_PAIR(CP_WIN) | A_BOLD);
    } else if (g->state == GAME_LOSE) {
        attron(COLOR_PAIR(CP_LOSE) | A_BOLD);
        mvprintw(row, 0, "  *** BOOM!  Game over ***");
        attroff(COLOR_PAIR(CP_LOSE) | A_BOLD);
    }
    row++;

    /* ── Controls ── */
    mvprintw(row,     0, "wasd/arrows:move x,y  <,>:z  SPC:reveal  f:flag  x:debug  r:settings  q:quit");
    mvprintw(row + 1, 0, "Cursor:(%d,%d,%d)  Legend: #=hidden F=flag .=0 1-9=adj A-Q=adj(10-26) X=mine *=mine(debug)",
             cur_x, cur_y, cur_z);
    row += 3;

    /* ── 3D iso (left) + flat layers (right, horizontal) ── */
    {
        int layer_w = 5 + b->width * 2;
        int gap     = 3;

        /* 3D iso on the left */
        attron(A_BOLD);
        mvprintw(row, 2, "3D  top=z%d front=y%d side=x%d bold=z%d",
                 b->depth-1, b->height-1, b->width-1, cur_z);
        attroff(A_BOLD);
        render_iso_section(b, cur_x, cur_y, cur_z, row + 1, 2, show);

        /* z-axis labels to the left of the iso view (col 0, 2 chars wide) */
        for (int bz = 0; bz < b->depth; bz++) {
            int lrow = (row + 1) + (b->height - 1) + (b->depth - 1 - bz) * 2;
            attron(bz == cur_z ? A_BOLD : A_DIM);
            mvprintw(lrow, 0, "z%d", bz);
            attroff(bz == cur_z ? A_BOLD : A_DIM);
        }

        /* Flat layers to the right of the iso view */
        int iso_w    = (b->width + b->height - 2) * 2 + 2;
        int flat_col = 2 + iso_w + gap;

        /* Build the list of up to 3 z-layers to display */
        int zs[3], nz = 0;
        if (cur_z > 0)          zs[nz++] = cur_z - 1;
        zs[nz++] = cur_z;
        if (cur_z < b->depth-1) zs[nz++] = cur_z + 1;

        /* Headers + x-axis: shifted down 1 to avoid overlapping the 3D label */
        for (int i = 0; i < nz; i++) {
            int z   = zs[i];
            int col = flat_col + i * (layer_w + gap);
            int ha  = (z == cur_z) ? A_BOLD | A_UNDERLINE : A_NORMAL;
            attron(ha);
            mvprintw(row + 1, col, "[z=%d]", z);
            attroff(ha);
            mvprintw(row + 2, col, "     ");
            for (int x = 0; x < b->width; x++) printw("%2d", x);
        }

        /* Data rows */
        for (int y = 0; y < b->height; y++) {
            for (int i = 0; i < nz; i++) {
                int z   = zs[i];
                int col = flat_col + i * (layer_w + gap);
                mvprintw(row + 3 + y, col, "y=%-2d", y);
                for (int x = 0; x < b->width; x++) {
                    const Cell *c  = &CELL_AT(b, x, y, z);
                    bool        ic = (x == cur_x && y == cur_y && z == cur_z);
                    draw_cell(c, ic, show);
                }
            }
        }

        int flat_rows = 3 + b->height;   /* 3D label row + [z] header + x-axis + data */
        int iso_rows  = 1 + iso_section_height(b);
        row += (flat_rows > iso_rows ? flat_rows : iso_rows) + 1;
    }

    refresh();
}
