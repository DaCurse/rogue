#ifndef _COLOR_C
#define _COLOR_C

#define COLOR_GRAY (8)

#define COLOR_UNKNOWN (-1)
#define COLOR_VOID (0)
#define COLOR_WALL (1)
#define COLOR_FLOOR (2)
#define COLOR_ROAD (3)
#define COLOR_PLAYER (4)

void init_colors() {
    assert(has_colors());
    start_color();

    init_color(COLOR_GRAY, 500, 500, 500);

    init_pair(COLOR_UNKNOWN, COLOR_BLACK, COLOR_WHITE);
    init_pair(COLOR_VOID, COLOR_BLACK, COLOR_BLACK);
    init_pair(COLOR_WALL, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_FLOOR, COLOR_GREEN, COLOR_BLACK);
    init_pair(COLOR_ROAD, COLOR_GRAY, COLOR_GRAY);
    init_pair(COLOR_PLAYER, COLOR_WHITE, COLOR_GREEN);
}

#endif // _COLOR_C