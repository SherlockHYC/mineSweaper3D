#include <ncurses.h>
#include <stdio.h>
#include "settings.h"

#define N_FIELDS 4   /* width, height, depth, mines */

void settings_clamp(Settings *s) {
    if (s->width  < BOARD_MIN) s->width  = BOARD_MIN;
    if (s->width  > BOARD_MAX) s->width  = BOARD_MAX;
    if (s->height < BOARD_MIN) s->height = BOARD_MIN;
    if (s->height > BOARD_MAX) s->height = BOARD_MAX;
    if (s->depth  < BOARD_MIN) s->depth  = BOARD_MIN;
    if (s->depth  > BOARD_MAX) s->depth  = BOARD_MAX;

    int max_mines = s->width * s->height * s->depth - 1;
    if (s->mines < 1)          s->mines = 1;
    if (s->mines > max_mines)  s->mines = max_mines;
}

/* ── Drawing helpers ─────────────────────────────────────── */

static void draw_field(int row, int col, const char *label,
                        const char *range, int value, bool selected) {
    if (selected) attron(A_REVERSE | A_BOLD);
    mvprintw(row, col, "  %-6s  (%s)  :  [ %3d ]  ", label, range, value);
    if (selected) attroff(A_REVERSE | A_BOLD);
}

/* ── Settings screen ─────────────────────────────────────── */

bool settings_screen(Settings *s) {
    int sel = 0;   /* 0=width 1=height 2=depth 3=mines */

    while (1) {
        int total     = s->width * s->height * s->depth;
        int max_mines = total - 1;
        int density   = (s->mines * 100) / total;

        clear();

        /* ── Title ── */
        int row = 1;
        attron(A_BOLD);
        mvprintw(row++, 4, "3D Minesweeper - Settings");
        attroff(A_BOLD);
        mvprintw(row++, 4, "---------------------------------");
        row++;

        /* ── Fields ── */
        char mine_range[16];
        snprintf(mine_range, sizeof(mine_range), "1-%-3d", max_mines);

        const char *labels[] = { "Width ", "Height", "Depth ", "Mines " };
        const char *ranges[] = { "5-8", "5-8", "5-8", mine_range };
        int         values[] = { s->width, s->height, s->depth, s->mines };

        for (int i = 0; i < N_FIELDS; i++) {
            if (i == 3) row++;   /* blank line before mines */
            draw_field(row++, 4, labels[i], ranges[i], values[i], i == sel);
        }

        /* ── Info ── */
        row++;
        mvprintw(row++, 4, "Board   : %d x %d x %d = %d cells",
                 s->width, s->height, s->depth, total);
        mvprintw(row++, 4, "Safe    : %d cells", total - s->mines);

        /* Density bar: 20 chars wide */
        int bar_fill = density / 5;   /* 100% → 20 chars */
        if (bar_fill > 20) bar_fill = 20;
        mvprintw(row, 4, "Density : %3d%%  [", density);
        attron(A_REVERSE);
        for (int i = 0; i < bar_fill;    i++) addch(' ');
        attroff(A_REVERSE);
        for (int i = bar_fill; i < 20;   i++) addch('-');
        addch(']');

        /* Difficulty hint */
        const char *hint = density < 13 ? "Easy"
                         : density < 20 ? "Medium"
                         : density < 25 ? "Hard"
                         :                "Expert";
        printw("  %s", hint);
        row += 2;

        /* ── Controls ── */
        mvprintw(row++, 4, "w/s  UP/DOWN : select field");
        mvprintw(row++, 4, "a/d  LT/RT   : change value  (hold for fast)");
        mvprintw(row++, 4, "Enter        : start game");
        mvprintw(row++, 4, "q            : quit");

        refresh();

        /* ── Input ── */
        int ch = getch();
        switch (ch) {
            case KEY_UP:   case 'w':
                sel = (sel - 1 + N_FIELDS) % N_FIELDS;
                break;
            case KEY_DOWN: case 's':
                sel = (sel + 1) % N_FIELDS;
                break;

            case KEY_LEFT: case 'a':
                switch (sel) {
                    case 0: s->width--;  break;
                    case 1: s->height--; break;
                    case 2: s->depth--;  break;
                    case 3: s->mines--;  break;
                }
                settings_clamp(s);
                break;

            case KEY_RIGHT: case 'd':
                switch (sel) {
                    case 0: s->width++;  break;
                    case 1: s->height++; break;
                    case 2: s->depth++;  break;
                    case 3: s->mines++;  break;
                }
                settings_clamp(s);
                break;

            case '\n': case '\r': case KEY_ENTER:
                return true;

            case 'q': case 'Q':
                return false;
        }
    }
}
