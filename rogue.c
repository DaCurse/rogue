#include <assert.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "color.c"
#include "ecs.c"
#include "floor.c"
#include "systems.c"

#define MAP_W (116)
#define MAP_H (36)
#define STATUS_H (1)
#define LOG_W (40)

#define MAX_ROOMS (10)
#define PLAYER_LIGHT_RADIUS (8)

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

void create_player(World *world, Floor *floor) {
    Entity player = world_create_entity(world);
    world->player = player;
    world->player_data = (Player){.light_radius = PLAYER_LIGHT_RADIUS};

    Room first_room = floor->rooms[0];
    Position starting_pos = {
        .x = first_room.x + 1 + rand() % (first_room.w - 2),
        .y = first_room.y + 1 + rand() % (first_room.h - 2),
    };
    Renderable player_render = {.glyph = '@', .color_pair = COLOR_PLAYER};
    world_add_position(world, player, starting_pos);
    world_add_renderable(world, player, player_render);
    world_add_combat_stats(world, player,
                           (CombatStats){
                               .name = "Player",
                               .hp = 100,
                               .max_hp = 100,
                               .attack = 10,
                               .defense = 0,
                           });

    floor_reveal_area(floor, starting_pos.x, starting_pos.y,
                      world->player_data.light_radius);
}

Entity create_enemy(World *world, Floor *floor) {
    Entity enemy = world_create_entity(world);
    Room room = floor->rooms[1 + rand() % (floor->room_count - 1)];
    Position enemy_pos = {.x = room.x + 1 + rand() % (room.w - 2),
                          .y = room.y + 1 + rand() % (room.h - 2)};
    Renderable enemy_render = {.glyph = 'E', .color_pair = COLOR_ENEMY};
    world_add_position(world, enemy, enemy_pos);
    world_add_renderable(world, enemy, enemy_render);
    world_add_combat_stats(world, enemy,
                           (CombatStats){
                               .name = "Enemy",
                               .hp = 20,
                               .max_hp = 20,
                               .attack = 5,
                               .defense = 2,
                           });
    world_add_collider(world, enemy, (Collider){.blocks_movement = true});

    return enemy;
}

int main(int argc, char **argv) {
    unsigned int seed;
    if (argc > 1) {
        seed = atoi(argv[1]);
    } else {
        seed = time(NULL);
    }
    srand(seed);

    // Initialize floor
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

    floor_fill_void(&floor);
    floor_generate_rooms(&floor, 8, 20);
    floor_build_walls(&floor);
    floor_connect_rooms(&floor);

    // Initialize ncurses
    initscr();
    init_colors();

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
    noecho();
    cbreak();
    curs_set(0);

    // Initialize world and player
    World world = {0};
    world.seed = seed;
    world.floor = &floor;
    create_player(&world, &floor);
    create_enemy(&world, &floor);

    world.map = (RenderContext){.win = map_win};
    world.log_window = (RenderContext){.win = log_win};
    world.status_bar = (RenderContext){.win = status_win};

    // Main game loop
    while (true) {
        system_render_map(&world);
        system_render_status_bar(&world);
        system_render_logs(&world);

        int ch = wgetch(map_win);
        if (ch == 'q')
            break;

        system_player_input(&world, ch);
        system_movement(&world);
        system_combat(&world);
        system_death(&world);
    }

    endwin();
    return 0;
}
