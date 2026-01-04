#include <assert.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "color.c"
#include "ecs.c"
#include "floor.c"
#include "systems.c"

#define MAP_W (120)
#define MAP_H (24)
#define STATUS_H (1)
#define LOG_W (24)

#define MAX_ROOMS (8)

Renderable tile_glyph(Floor *f, int x, int y) {
    switch (tile_at(f, x, y)) {
    case TILE_VOID:
        return (Renderable){.glyph = ' ', .color_pair = COLOR_VOID};
    case TILE_FLOOR:
        return (Renderable){.glyph = '.', .color_pair = COLOR_FLOOR};
    case TILE_ROAD:
        return (Renderable){.glyph = ' ', .color_pair = COLOR_ROAD};
    case TILE_WALL:
        switch (f->walls[x + f->width * y]) {
        case WALL_VERTICAL:
            return (Renderable){.glyph = '|', .color_pair = COLOR_WALL};
        case WALL_HORIZONTAL:
            return (Renderable){.glyph = '_', .color_pair = COLOR_WALL};
        case WALL_CORNER:
            return (Renderable){.glyph = '+', .color_pair = COLOR_WALL};
        default:
            return (Renderable){.glyph = '?', .color_pair = COLOR_UNKNOWN};
        }
    default:
        return (Renderable){.glyph = '?', .color_pair = COLOR_UNKNOWN};
    }
}

void draw_map(WINDOW *win, Floor *f, World *w) {
    werase(win);

    for (int y = 0; y < f->height; y++) {
        for (int x = 0; x < f->width; x++) {
            if (!f->fog_of_war[x + f->width * y])
                continue;

            Renderable r = tile_glyph(f, x, y);
            mvwaddch(win, y, x, r.glyph | COLOR_PAIR(r.color_pair));
        }
    }

    system_render(win, w);

    wrefresh(win);
}

void draw_status_bar(WINDOW *win) {
    werase(win);

    wbkgd(win, COLOR_PAIR(COLOR_STATUS));
    mvwprintw(win, 0, 1, "Level: 1 Floor: 1 Gold: 0");

    wrefresh(win);
}

void draw_logs(WINDOW *win) {
    werase(win);
    box(win, 0, 0);
    mvwprintw(win, 0, 2, " Journal ");
    wrefresh(win);
}

int main() {
    Tile tiles[MAP_H * MAP_W];
    WallType walls[MAP_H * MAP_W];
    bool fog_of_war[MAP_H * MAP_W] = {0};
    Room rooms[MAX_ROOMS];
    Floor floor = {.width = MAP_W,
                   .height = MAP_H,

                   .tiles = tiles,
                   .walls = walls,
                   .fog_of_war = fog_of_war,

                   .room_count = 0,
                   .max_rooms = MAX_ROOMS,
                   .rooms = rooms};

    srand(time(NULL));
    floor_fill_void(&floor);
    floor_generate_rooms(&floor, 8, 20);
    floor_build_walls(&floor);

    World world = {0};
    Entity player = world_create_entity(&world);
    Room first_room = floor.rooms[0];
    Position starting_pos = {
        .x = first_room.x + 1 + rand() % (first_room.w - 2),
        .y = first_room.y + 1 + rand() % (first_room.h - 2)};
    Renderable player_render = {.glyph = '@', .color_pair = COLOR_PLAYER};
    world_add_position(&world, player, starting_pos);
    world_add_renderable(&world, player, player_render);
    floor_reveal_area(&floor, starting_pos.x, starting_pos.y, 5);

    initscr();

    WINDOW *map_win = newwin(floor.height, floor.width, 0, 0);
    WINDOW *log_win = newwin(floor.height, LOG_W, 0, floor.width);
    WINDOW *status_win = newwin(STATUS_H, floor.width + LOG_W, floor.height, 0);

    keypad(map_win, true);
    init_colors();
    noecho();
    cbreak();
    curs_set(0);

    while (true) {
        draw_map(map_win, &floor, &world);
        draw_status_bar(status_win);
        draw_logs(log_win);

        int ch = wgetch(map_win);
        if (ch == 'q')
            break;

        system_player_input(&world, player, ch, &floor);
    }

    endwin();
    return 0;
}
