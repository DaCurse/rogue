#ifndef COLOR_H
#define COLOR_H

typedef enum {
    COLOR_PAIR_UNKNOWN = -1,
    COLOR_PAIR_VOID = 0,
    COLOR_PAIR_WALL,
    COLOR_PAIR_FLOOR,
    COLOR_PAIR_ROAD,
    COLOR_PAIR_PLAYER,
    COLOR_PAIR_STATUS,
    COLOR_PAIR_ENEMY,
    COLOR_PAIR_EXIT
} ColorPair;

void init_colors(void);

#endif // COLOR_H
