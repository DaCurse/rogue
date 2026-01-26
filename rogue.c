#include <assert.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "color.h"
#include "config.h"
#include "consumable.h"
#include "floor.h"
#include "game.h"
#include "item.h"
#include "systems.h"
#include "terminal.h"
#include "utils.h"
#include "world.h"

#include "color.c"
#include "consumable.c"
#include "enemy.c"
#include "floor.c"
#include "game.c"
#include "item.c"
#include "systems.c"
#include "utils.c"
#include "world.c"

WINDOW *create_window(int h, int w, int y, int x) {
    assert(h > 0 && w > 0);

    WINDOW *win = newwin(h, w, y, x);
    if (win == NULL) {
        endwin();
        fprintf(stderr, "Error: Window creation failed at %d,%d (%dx%d).\n", y,
                x, w, h);
        exit(1);
    }
    return win;
}

TileWithFog map_data[MAP_H * MAP_W] = {0};
WallType walls[MAP_H * MAP_W] = {0};
Room rooms[MAX_ROOMS];
Floor level_floor = {.width = MAP_W,
                     .height = MAP_H,

                     .data = map_data,
                     .walls = walls,

                     .room_count = 0,
                     .max_rooms = MAX_ROOMS,
                     .rooms = rooms};

World world = {0};

int main(int argc, char **argv) {
    // Ensure terminal is large enough
    ensure_terminal_size(MAP_W, MAP_H + LOG_H + STATUS_H);

    unsigned int seed;
    if (argc > 1) {
        seed = (unsigned int)strtoul(argv[1], NULL, 10);
    } else {
        seed = (unsigned int)time(NULL);
    }
    srand(seed);

    // Initialize world and player
    char player_name[NAME_MAX_LENGTH];
    printf("What is your name, adventurer? ");
    if (fgets(player_name, sizeof(player_name), stdin) != NULL) {
        filter_non_alpha(player_name);
    }
    if (player_name[0] == '\0') {
        strcpy(player_name, PLAYER_DEFAULT_NAME);
    }

    world.seed = seed;
    world.floor = &level_floor;
    create_player(&world, player_name);
    setup_new_level(&world);

    // Initialize ncurses
    initscr();
    init_colors();

    WINDOW *log_win = create_window(LOG_H, world.floor->width, 0, 0);
    WINDOW *map_win =
        create_window(world.floor->height, world.floor->width, LOG_H, 0);
    WINDOW *status_win = create_window(STATUS_H, world.floor->width,
                                       world.floor->height + LOG_H, 0);

    world.map.win = map_win;
    world.log_window.win = log_win;
    world.status_bar.win = status_win;

    keypad(map_win, true);
    noecho();
    cbreak();
    curs_set(0);

    // Main game loop
    for (;;) {
        system_render_map(&world);
        system_render_status_bar(&world);
        system_render_logs(&world);

        int ch = wgetch(map_win);
        if (ch == 'q' || world.player_data.game_over) {
            break;
        }
        world.player_data.input = ch;

        uint32_t turn_count_before = world.player_data.turn_count;
        system_player_input(&world);

        if (world.player_data.turn_count > turn_count_before) {
            system_tick_turn_delay(&world);
            system_ai(&world);
            system_movement(&world);
            system_combat(&world);
            system_pickup_item(&world);
            system_exit_room(&world);
            system_death(&world);
        }
    }

    endwin();

    if (world.player_data.game_over) {
        printf("\nGame Over!\n");
        printf("%s reached Floor %d.\n", world.names[world.player].name,
               world.player_data.floor);

#ifdef _WIN32
        if (is_double_clicked()) {
            printf("Press any key to exit...");
            getchar();
        }
#endif
    }

    return 0;
}
