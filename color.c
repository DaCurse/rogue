#include <ncurses.h>

#include "color.h"

static short custom_color = COLOR_WHITE;
static short COLOR_GRAY;

void init_colors(void) {
    assert(has_colors());
    start_color();

    if (can_change_color()) { // custom colors
        COLOR_GRAY = ++custom_color;

        init_color(COLOR_GRAY, 500, 500, 500);
    } else { // fallbacks
        COLOR_GRAY = COLOR_WHITE;
    }

    init_pair(COLOR_PAIR_UNKNOWN, COLOR_BLACK, COLOR_WHITE);
    init_pair(COLOR_PAIR_VOID, COLOR_BLACK, COLOR_BLACK);
    init_pair(COLOR_PAIR_WALL, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_PAIR_FLOOR, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_PAIR_ROAD, COLOR_GRAY, COLOR_GRAY);
    init_pair(COLOR_PAIR_PLAYER, COLOR_WHITE, COLOR_GREEN);
    init_pair(COLOR_PAIR_STATUS, COLOR_WHITE, COLOR_YELLOW);
    init_pair(COLOR_PAIR_ENEMY, COLOR_WHITE, COLOR_RED);
    init_pair(COLOR_PAIR_EXIT, COLOR_GRAY, COLOR_BLACK);
}