#include <assert.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "color.h"
#include "config.h"
#include "floor.h"
#include "game.h"
#include "systems.h"
#include "utils.h"
#include "world.h"

#include "color.c"
#include "enemy.c"
#include "floor.c"
#include "game.c"
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

int main(int argc, char **argv) {
    unsigned int seed;
    if (argc > 1) {
        seed = (unsigned int)strtoul(argv[1], NULL, 10);
    } else {
        seed = (unsigned int)time(NULL);
    }
    srand(seed);

    char player_name[PLAYER_NAME_MAX_LENGTH];
    printf("What is your name, adventurer? ");
    if (fgets(player_name, sizeof(player_name), stdin) != NULL) {
        filter_non_alpha(player_name);
    }
    if (player_name[0] == '\0') {
        strcpy(player_name, PLAYER_DEFAULT_NAME);
    }

    // Initialize floor
    TileWithFog map_data[MAP_H * MAP_W] = {0};
    WallType walls[MAP_H * MAP_W] = {0};
    Room rooms[MAX_ROOMS];
    Floor floor = {.width = MAP_W,
                   .height = MAP_H,

                   .data = map_data,
                   .walls = walls,

                   .room_count = 0,
                   .max_rooms = MAX_ROOMS,
                   .rooms = rooms};

    // Initialize ncurses
    initscr();
    init_colors();

    if (LINES < MAP_H + STATUS_H + LOG_H || COLS < MAP_W) {
        endwin();
        fprintf(stderr, "Terminal too small: %dx%d\n", COLS, LINES);
        fprintf(stderr, "Required: %dx%d\n", MAP_W, MAP_H + STATUS_H + LOG_H);
        return 1;
    }

    WINDOW *log_win = create_window(LOG_H, floor.width, 0, 0);
    WINDOW *map_win = create_window(floor.height, floor.width, LOG_H, 0);
    WINDOW *status_win =
        create_window(STATUS_H, floor.width, floor.height + LOG_H, 0);

    keypad(map_win, true);
    noecho();
    cbreak();
    curs_set(0);

    // Initialize world and player
    World world = {0};
    world.seed = seed;
    world.floor = &floor;
    create_player(&world, player_name);
    setup_new_level(&world);

    world.map.win = map_win;
    world.log_window.win = log_win;
    world.status_bar.win = status_win;

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

        int turn_count_before = world.player_data.turn_count;
        system_player_input(&world);

        if (world.player_data.turn_count > turn_count_before) {
            system_tick_turn_delay(&world);
            system_ai(&world);
            system_movement(&world);
            system_combat(&world);
            system_exit_room(&world);
            system_death(&world);
        }
    }

    endwin();

    if (world.player_data.game_over) {
        printf("\nGame Over!\n");
        printf("%s reached Floor %d.\n", world.combat_stats[world.player].name,
               world.player_data.floor);
    }

    return 0;
}
