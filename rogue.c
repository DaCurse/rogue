#include <assert.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "color.c"
#include "draw.c"
#include "ecs.c"
#include "floor.c"
#include "systems.c"

#define MAP_W (120)
#define MAP_H (24)
#define STATUS_H (1)
#define LOG_W (24)

#define MAX_ROOMS (8)

WINDOW *create_window(int h, int w, int y, int x) {
    WINDOW *win = newwin(h, w, y, x);
    if (win == NULL) {
        endwin();
        fprintf(stderr, "Error: Window creation failed at %d,%d (%dx%d).\n", y,
                x, w, h);
        exit(1);
    }
    return win;
}

int main() {
    // Create game objects
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

    // Initialize ncurses
    initscr();

    if (LINES < MAP_H + STATUS_H || COLS < MAP_W + LOG_W) {
        endwin();
        fprintf(stderr, "Terminal too small: %dx%d\n", COLS, LINES);
        fprintf(stderr, "Required: %dx%d\n", MAP_W + LOG_W, MAP_H + STATUS_H);
        return 1;
    }

    WINDOW *map_win = create_window(floor.height, floor.width, 0, 0);
    WINDOW *log_win = create_window(floor.height, LOG_W, 0, floor.width);
    WINDOW *status_win =
        create_window(STATUS_H, floor.width + LOG_W, floor.height, 0);

    keypad(map_win, true);
    init_colors();
    noecho();
    cbreak();
    curs_set(0);

    // Main game loop
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
