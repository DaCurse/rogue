#include "floor.h"

#include <assert.h>
#include <stdlib.h>

#include "config.h"
#include "utils.h"

bool in_bounds(Floor *f, int x, int y) {
    return x >= 0 && y >= 0 && x < f->width && y < f->height;
}

Tile tile_at(Floor *f, int x, int y) {
    if (!in_bounds(f, x, y))
        return TILE_WALL;

    return f->data[x + f->width * y].tile;
}

void floor_fill_void(Floor *f) {
    for (int i = 0; i < f->width * f->height; i++) {
        f->data[i].tile = TILE_VOID;
        f->walls[i] = WALL_UNSET;
    }
}

void carve_room(Floor *f, Room r) {
    assert(f->room_count < f->max_rooms);

    for (int y = r.y; y < r.y + r.h; y++) {
        for (int x = r.x; x < r.x + r.w; x++) {
            int i = x + f->width * y;
            f->data[i].tile = TILE_FLOOR;
        }
    }

    f->rooms[f->room_count++] = r;
}

// Helper to get 1D index from 2D coordinates, abstracting horizontal/vertical
// orientation
static int get_idx(Floor *f, int u, int v, bool horizontal) {
    int x = horizontal ? u : v;
    int y = horizontal ? v : u;
    return x + f->width * y;
}

// Checks if a tile is safe to carve (not a specific bad wall type or a corner)
static bool is_safe(Floor *f, int idx, WallType bad_wall) {
    if (idx < 0 || idx >= f->width * f->height)
        return false;
    return f->walls[idx] != bad_wall && f->walls[idx] != WALL_CORNER;
}

// Determines if we should skip carving the current tile to avoid creating a
// "stub" or unconnected road segment, particularly when a perpendicular
// corridor would have shifted its connection point.
static bool should_skip_stub(Floor *f, int u, int v, bool horizontal, int start,
                             int end, WallType perp_bad_wall) {
    int i = get_idx(f, u, v, horizontal);
    if (is_safe(f, i, perp_bad_wall))
        return false;

    if (u == end && u > start) {
        int prev_i = get_idx(f, u - 1, v, horizontal);
        if (is_safe(f, prev_i, perp_bad_wall))
            return true;
    } else if (u == start && u < end) {
        int prev_i = get_idx(f, u - 1, v, horizontal);
        if (!is_safe(f, prev_i, perp_bad_wall)) {
            int next_i = get_idx(f, u + 1, v, horizontal);
            if (is_safe(f, next_i, perp_bad_wall))
                return true;
        }
    }
    return false;
}

// Calculates the necesary offset (-1, 0, or 1) for the current path segment
// to avoid obstacles. Includes "momentum" logic to prevent zigzagging.
static int calculate_offset(Floor *f, int u, int v, bool horizontal,
                            int limit_v, WallType bad_wall, int prev_offset,
                            int end) {
    int i = get_idx(f, u, v, horizontal);

    // If current is bad, we MUST nudge
    if (!is_safe(f, i, bad_wall)) {
        if (v > 0 && is_safe(f, get_idx(f, u, v - 1, horizontal), bad_wall))
            return -1;
        if (v < limit_v - 1 &&
            is_safe(f, get_idx(f, u, v + 1, horizontal), bad_wall))
            return 1;
        return 0; // Stuck, blast through
    }

    // Current is safe. Check momentum to avoid zig-zag.
    if (prev_offset != 0 && u < end) {
        int next_i = get_idx(f, u + 1, v, horizontal);
        // If next is bad, and staying at prev_offset is safe, stay there.
        if (!is_safe(f, next_i, bad_wall)) {
            int keep_i = get_idx(f, u, v + prev_offset, horizontal);
            if (is_safe(f, keep_i, bad_wall))
                return prev_offset;
        }
    }

    return 0;
}

// Fills in diagonal gaps when the corridor path shifts offset, ensuring
// connectivity.
static void draw_connectivity(Floor *f, int u, int v, bool horizontal,
                              int offset, int prev_offset, WallType bad_wall) {
    // Optimization: if returning to 0, check if already connected
    if (offset == 0) {
        int back_i = get_idx(f, u - 1, v, horizontal);
        if (f->data[back_i].tile == TILE_ROAD ||
            f->data[back_i].tile == TILE_FLOOR)
            return;
    }

    int fill_u = u;
    int fill_v = v;

    if (prev_offset != 0) {
        fill_v = v + prev_offset;
    } else {
        fill_u = u - 1;
        fill_v = v + offset;
    }

    int fill_i = get_idx(f, fill_u, fill_v, horizontal);
    if (is_safe(f, fill_i, bad_wall)) {
#ifdef DEBUG_PAINT_ROADS
        f->data[fill_i].tile = TILE_ROAD;
#else
        if (f->data[fill_i].tile != TILE_FLOOR)
            f->data[fill_i].tile = TILE_ROAD;
#endif
    }
}

// Main logic for safely carving a straight line that can nudge around
// obstacles.
static void carve_safe_line(Floor *f, int u1, int u2, int v, bool horizontal) {
    int start = (u1 < u2 ? u1 : u2);
    int end = (u1 > u2 ? u1 : u2);

    WallType bad_wall = horizontal ? WALL_HORIZONTAL : WALL_VERTICAL;
    WallType perp_bad_wall = horizontal ? WALL_VERTICAL : WALL_HORIZONTAL;
    int limit_v = horizontal ? f->height : f->width;

    int prev_offset = 0;

    for (int u = start; u <= end; u++) {
        if (should_skip_stub(f, u, v, horizontal, start, end, perp_bad_wall))
            continue;

        int offset = calculate_offset(f, u, v, horizontal, limit_v, bad_wall,
                                      prev_offset, end);

        int current_i = get_idx(f, u, v + offset, horizontal);

#ifdef DEBUG_PAINT_ROADS
        f->data[current_i].tile = TILE_ROAD;
#else
        if (f->data[current_i].tile != TILE_FLOOR)
            f->data[current_i].tile = TILE_ROAD;
#endif

        if (u > start && offset != prev_offset) {
            draw_connectivity(f, u, v, horizontal, offset, prev_offset,
                              bad_wall);
        }
        prev_offset = offset;
    }
}

void carve_horizontal_corridor(Floor *f, int x1, int x2, int y) {
    carve_safe_line(f, x1, x2, y, true);
}

void carve_vertical_corridor(Floor *f, int y1, int y2, int x) {
    carve_safe_line(f, y1, y2, x, false);
}

bool rooms_intersect(Room a, Room b) {
    int gap = 2;
    return (a.x - gap <= b.x + b.w && a.x + a.w + gap >= b.x &&
            a.y - gap <= b.y + b.h && a.y + a.h + gap >= b.y);
}

void floor_generate_rooms(Floor *f, int room_min_size, int room_max_size) {
    f->room_count = 0;
    for (int i = 0; i < f->max_rooms; i++) {
        Room r;
        r.w = random_int(room_min_size, room_max_size);
        r.h = random_int(room_min_size / CELL_ASPECT_RATIO,
                         room_max_size / CELL_ASPECT_RATIO);

        int max_x = f->width - r.w - 2;
        int max_y = f->height - r.h - 2;
        if (max_x <= 2 || max_y <= 2)
            continue;

        r.x = random_int(2, max_x);
        r.y = random_int(2, max_y);

        bool failed = false;
        for (int j = 0; j < f->room_count; j++) {
            if (rooms_intersect(r, f->rooms[j])) {
                failed = true;
                break;
            }
        }

        if (failed)
            continue;

        carve_room(f, r);
    }
}

int compare_rooms(const void *a, const void *b) {
    Room *r1 = (Room *)a;
    Room *r2 = (Room *)b;
    return r1->x - r2->x;
}

void floor_connect_rooms(Floor *f) {
    qsort(f->rooms, (size_t)f->room_count, sizeof(Room), compare_rooms);

    for (int i = 1; i < f->room_count; i++) {
        Room r = f->rooms[i];
        Room prev = f->rooms[i - 1];

        int x1 = r.x + r.w / 2;
        int y1 = r.y + r.h / 2;
        int x2 = prev.x + prev.w / 2;
        int y2 = prev.y + prev.h / 2;

        if (chance(0.5f)) {
            carve_horizontal_corridor(f, x1, x2, y1);
            carve_vertical_corridor(f, y1, y2, x2);
        } else {
            carve_vertical_corridor(f, y1, y2, x1);
            carve_horizontal_corridor(f, x1, x2, y2);
        }
    }
}

void floor_build_walls(Floor *f) {
    for (int y = 0; y < f->height; y++) {
        for (int x = 0; x < f->width; x++) {
            int i = x + f->width * y;
            if (f->data[i].tile != TILE_VOID)
                continue;

            bool floor_up = tile_at(f, x, y - 1) == TILE_FLOOR;
            bool floor_down = tile_at(f, x, y + 1) == TILE_FLOOR;
            bool floor_left = tile_at(f, x - 1, y) == TILE_FLOOR;
            bool floor_right = tile_at(f, x + 1, y) == TILE_FLOOR;

            bool floor_nw = tile_at(f, x - 1, y - 1) == TILE_FLOOR;
            bool floor_ne = tile_at(f, x + 1, y - 1) == TILE_FLOOR;
            bool floor_sw = tile_at(f, x - 1, y + 1) == TILE_FLOOR;
            bool floor_se = tile_at(f, x + 1, y + 1) == TILE_FLOOR;

            if (!floor_up && !floor_down && !floor_left && !floor_right &&
                !floor_nw && !floor_ne && !floor_sw && !floor_se)
                continue;

            f->data[i].tile = TILE_WALL;

            int count_horiz = floor_left + floor_right;

            if (floor_down || floor_up) {
                f->walls[i] = WALL_HORIZONTAL;
            } else if (count_horiz) {
                f->walls[i] = WALL_VERTICAL;
            } else {
                f->walls[i] = WALL_CORNER;
            }
        }
    }
}

void floor_reveal_area(Floor *f, int x, int y, int radius) {
    floor_reveal_filtered(f, x, y, radius, ~0u);
}

void floor_reveal_filtered(Floor *f, int x, int y, int radius,
                           unsigned int mask) {
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy <= radius * radius) {
                int nx = x + dx;
                int ny = y + dy;
                if (!in_bounds(f, nx, ny))
                    continue;

                Tile t = tile_at(f, nx, ny);
                // Check if the tile type matches the bitmask.
                // (1 << t) creates a bit with the index of the tile enum value.
                if (!(mask & (1 << t)))
                    continue;

                f->data[nx + f->width * ny].fog = true;
            }
        }
    }
}

int floor_find_room(Floor *f, int x, int y) {
    for (int i = 0; i < f->room_count; i++) {
        Room *r = &f->rooms[i];
        if (x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h) {
            return i;
        }
    }
    return -1;
}

void floor_reveal_room(Floor *f, int room_index) {
    if (room_index < 0 || room_index >= f->room_count)
        return;

    Room *r = &f->rooms[room_index];

    for (int y = r->y - 1; y <= r->y + r->h; y++) {
        for (int x = r->x - 1; x <= r->x + r->w; x++) {
            if (in_bounds(f, x, y)) {
                f->data[x + f->width * y].fog = true;
            }
        }
    }
}

Room *floor_random_room(Floor *f) {
    if (f->room_count == 0)
        return NULL;

    int room_idx = random_int(0, f->room_count - 1);
    return &f->rooms[room_idx];
}

Room *floor_random_room_excl(Floor *f, int excluded_idx) {
    if (f->room_count == 0)
        return NULL;

    // If exclusion is out of bounds, just pick any random room
    if (excluded_idx < 0 || excluded_idx >= f->room_count) {
        return &f->rooms[random_int(0, f->room_count - 1)];
    }

    if (f->room_count == 1) {
        // Since excluded_idx is valid (0), we must exclude the only room
        return NULL;
    }

    // Pick from the range of indices [0, room_count - 2]
    // Virtually, this represents the array of indices with 'excluded_idx'
    // removed.
    int idx = random_int(0, f->room_count - 2);
    if (idx >= excluded_idx) {
        idx++;
    }

    return &f->rooms[idx];
}
