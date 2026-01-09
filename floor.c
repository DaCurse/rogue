#include "floor.h"

bool in_bounds(Floor *f, int x, int y) {
    return x >= 0 && y >= 0 && x < f->width && y < f->height;
}

Tile tile_at(Floor *f, int x, int y) {
    if (!in_bounds(f, x, y))
        return TILE_WALL;

    return f->tiles[x + f->width * y];
}

void floor_fill_void(Floor *f) {
    for (int i = 0; i < f->width * f->height; i++) {
        f->tiles[i] = TILE_VOID;
        f->walls[i] = WALL_UNSET;
    }
}

void carve_room(Floor *f, Room r) {
    assert(f->room_count < f->max_rooms);

    for (int y = r.y; y < r.y + r.h; y++) {
        for (int x = r.x; x < r.x + r.w; x++) {
            int i = x + f->width * y;
            f->tiles[i] = TILE_FLOOR;
        }
    }

    f->rooms[f->room_count++] = r;
}

void carve_horizontal_corridor(Floor *f, int x1, int x2, int y) {
    for (int x = (x1 < x2 ? x1 : x2); x <= (x1 > x2 ? x1 : x2); x++) {
        int i = x + f->width * y;
        if (f->tiles[i] != TILE_FLOOR) {
            f->tiles[i] = TILE_ROAD;
        }
    }
}

void carve_vertical_corridor(Floor *f, int y1, int y2, int x) {
    for (int y = (y1 < y2 ? y1 : y2); y <= (y1 > y2 ? y1 : y2); y++) {
        int i = x + f->width * y;
        if (f->tiles[i] != TILE_FLOOR) {
            f->tiles[i] = TILE_ROAD;
        }
    }
}

bool rooms_intersect(Room a, Room b) {
    return (a.x <= b.x + b.w && a.x + a.w >= b.x && a.y <= b.y + b.h &&
            a.y + a.h >= b.y);
}

void floor_generate_rooms(Floor *f, int room_min_size, int room_max_size) {
    f->room_count = 0;
    for (int i = 0; i < f->max_rooms; i++) {
        Room r;
        r.w = room_min_size + rand() % (room_max_size - room_min_size + 1);
        r.h =
            room_min_size + rand() % ((room_max_size / 2) - room_min_size + 1);

        int max_x = f->width - r.w - 1;
        int max_y = f->height - r.h - 1;
        if (max_x <= 1 || max_y <= 1)
            continue;
        r.x = 1 + rand() % max_x;
        r.y = 1 + rand() % max_y;

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

void floor_connect_rooms(Floor *f) {
    for (int i = 1; i < f->room_count; i++) {
        Room r = f->rooms[i];
        Room prev = f->rooms[i - 1];

        int x1 = r.x + r.w / 2;
        int y1 = r.y + r.h / 2;
        int x2 = prev.x + prev.w / 2;
        int y2 = prev.y + prev.h / 2;

        if (rand() % 2) {
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
            if (f->tiles[i] != TILE_VOID)
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

            f->tiles[i] = TILE_WALL;

            int count_horiz = floor_left + floor_right;
            int count_vert = floor_up + floor_down;

            if (count_vert) {
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
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy <= radius * radius) {
                int nx = x + dx;
                int ny = y + dy;
                if (in_bounds(f, nx, ny)) {
                    f->fog_of_war[nx + f->width * ny] = true;
                }
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
                f->fog_of_war[x + f->width * y] = true;
            }
        }
    }
}
