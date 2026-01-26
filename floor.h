#ifndef FLOOR_H
#define FLOOR_H

#include <assert.h>
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

static inline bool in_bounds(const Floor *f, int x, int y) {
    assert(f->width >= 0 && f->height >= 0);
    return (unsigned)x < (unsigned)f->width &&
           (unsigned)y < (unsigned)f->height;
}

static inline Tile tile_at(const Floor *f, int x, int y) {
    if (!in_bounds(f, x, y))
        return TILE_WALL;

    return f->data[x + f->width * y].tile;
}

void floor_fill_void(Floor *f);
void floor_generate_rooms(Floor *f, int room_min_size, int room_max_size);
void floor_connect_rooms(Floor *f);
void floor_build_walls(Floor *f);

void floor_reveal_filtered(Floor *f, int x, int y, int radius,
                           unsigned int mask);

static inline void floor_reveal_area(Floor *f, int x, int y, int radius) {
    floor_reveal_filtered(f, x, y, radius, ~0u);
}

int floor_find_room(Floor *f, int x, int y);
void floor_reveal_room(Floor *f, int room_index);
Room *floor_random_room(Floor *f);
Room *floor_random_room_excl(Floor *f, int excluded_idx);

#endif // FLOOR_H
