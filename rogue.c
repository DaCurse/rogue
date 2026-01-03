#include <assert.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "color.c"
#include "ecs.c"
#include "floor.c"
#include "systems.c"

#define MAX_ROOMS (5)
#define MAP_W (120)
#define MAP_H (24)

void draw(Floor *f) {
    for (int y = 0; y < f->height; y++) {
        for (int x = 0; x < f->width; x++) {
            if (!f->fog_of_war[x + f->width * y])
                continue;

            Renderable r = tile_glyph(f, x, y);
            mvaddch(y, x, r.glyph | COLOR_PAIR(r.color_pair));
        }
    }
}

int main() {
    Tile tiles[MAP_H * MAP_W];
    WallType walls[MAP_H * MAP_W];
    bool fog_of_war[MAP_H * MAP_W] = {0};
    Room rooms[MAX_ROOMS];
    Floor f = {.width = MAP_W,
               .height = MAP_H,

               .tiles = tiles,
               .walls = walls,
               .fog_of_war = fog_of_war,

               .room_count = 0,
               .max_rooms = MAX_ROOMS,
               .rooms = rooms};

    srand(time(NULL));
    floor_fill_void(&f);
    floor_generate_rooms(&f, 8, 20);
    floor_build_walls(&f);

    World world = {0};
    Entity player = world_create_entity(&world);
    Room first_room = f.rooms[0];
    Position starting_pos = {
        .x = first_room.x + 1 + rand() % (first_room.w - 2),
        .y = first_room.y + 1 + rand() % (first_room.h - 2)};
    Renderable player_render = {.glyph = '@', .color_pair = COLOR_PLAYER};
    world_add_position(&world, player, starting_pos);
    world_add_renderable(&world, player, player_render);
    floor_reveal_area(&f, starting_pos.x, starting_pos.y, 5);

    initscr();
    init_colors();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, true);

    while (true) {
        clear();

        draw(&f);
        system_render(&world);

        refresh();

        char ch = getch();
        if (ch == 'q')
            break;

        system_player_input(&world, player, ch, &f);
    }

    endwin();
    return 0;
}
