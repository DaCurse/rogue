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

    init_pair(COLOR_UNKNOWN, COLOR_BLACK, COLOR_WHITE);
    init_pair(COLOR_VOID, COLOR_BLACK, COLOR_BLACK);
    init_pair(COLOR_WALL, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_FLOOR, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_ROAD, COLOR_GRAY, COLOR_GRAY);
    init_pair(COLOR_PLAYER, COLOR_WHITE, COLOR_GREEN);
    init_pair(COLOR_STATUS, COLOR_WHITE, COLOR_YELLOW);
    init_pair(COLOR_ENEMY, COLOR_WHITE, COLOR_RED);
}