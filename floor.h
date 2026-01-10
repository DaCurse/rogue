#ifndef FLOOR_H
#define FLOOR_H

#include <stdbool.h>

// Terminal characters are ~2x taller than wide
#define CELL_ASPECT_RATIO 2

typedef enum { TILE_VOID, TILE_WALL, TILE_FLOOR, TILE_ROAD } Tile;

typedef enum {
    WALL_UNSET,
    WALL_HORIZONTAL,
    WALL_VERTICAL,
    WALL_CORNER
} WallType;

typedef struct {
    int x, y;
    int w, h;
} Room;

typedef struct {
    int width, height;

    Tile *tiles;
    WallType *walls;
    bool *fog_of_war;

    int room_count;
    int max_rooms;
    Room *rooms;
} Floor;

bool in_bounds(Floor *f, int x, int y);
Tile tile_at(Floor *f, int x, int y);
void floor_fill_void(Floor *f);
void carve_room(Floor *f, Room r);
void carve_horizontal_corridor(Floor *f, int x1, int x2, int y);
void carve_vertical_corridor(Floor *f, int y1, int y2, int x);
bool rooms_intersect(Room a, Room b);
void floor_generate_rooms(Floor *f, int room_min_size, int room_max_size);
void floor_connect_rooms(Floor *f);
void floor_build_walls(Floor *f);
void floor_reveal_area(Floor *f, int x, int y, int radius);
int floor_find_room(Floor *f, int x, int y);
void floor_reveal_room(Floor *f, int room_index);

#endif // FLOOR_H
