#ifndef FLOOR_H
#define FLOOR_H

#include <stdbool.h>

// Terminal characters are ~2x taller than wide
#define CELL_ASPECT_RATIO 2

typedef enum {
    TILE_VOID,
    TILE_WALL,
    TILE_FLOOR,
    TILE_ROAD,
} Tile;

typedef enum {
    WALL_UNSET,
    WALL_HORIZONTAL,
    WALL_VERTICAL,
    WALL_CORNER,
} WallType;

typedef struct {
    int x, y;
    int w, h;
} Room;

typedef struct {
    Tile tile;
    bool fog;
} TileWithFog;

typedef struct {
    int width, height;

    TileWithFog *data;
    WallType *walls;

    int room_count;
    int max_rooms;
    Room *rooms;
} Floor;

inline bool in_bounds(Floor *f, int x, int y);
inline Tile tile_at(Floor *f, int x, int y);
void floor_fill_void(Floor *f);
void floor_generate_rooms(Floor *f, int room_min_size, int room_max_size);
void floor_connect_rooms(Floor *f);
void floor_build_walls(Floor *f);
inline void floor_reveal_area(Floor *f, int x, int y, int radius);
void floor_reveal_filtered(Floor *f, int x, int y, int radius,
                           unsigned int mask);
int floor_find_room(Floor *f, int x, int y);
void floor_reveal_room(Floor *f, int room_index);
Room *floor_random_room(Floor *f);
Room *floor_random_room_excl(Floor *f, int excluded_idx);

#endif // FLOOR_H
