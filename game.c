#include "game.h"

#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "enemy.h"
#include "utils.h"

void create_player(World *world, char *name) {
    Entity player = world_create_entity(world);
    world->player = player;
    world->player_data = (PlayerData){
        .room_id = 0,
        .floor = 0,
    };

    Renderable player_render = {.glyph = '@', .color_pair = COLOR_PAIR_PLAYER};
    CombatStats combat_stats = {
        .hp = 50,
        .max_hp = 50,
        .attack = 10,
        .defense = 0,
    };
    snprintf(combat_stats.name, sizeof(combat_stats.name), "%s", name);

    // Position will be set in setup_new_level
    world_add_renderable(world, player, player_render);
    world_add_combat_stats(world, player, combat_stats);
}

void add_room_exit(World *world) {
    Room *room =
        floor_random_room_excl(world->floor, world->player_data.room_id);
    if (room == NULL) {
        return;
    }

    Entity exit_entity = world_create_entity(world);
    Position exit_pos;
    if (world_get_random_unoccupied_in_room(world, room, &exit_pos)) {
        Renderable exit_render = {.glyph = 'H', .color_pair = COLOR_PAIR_EXIT};
        world_add_position(world, exit_entity, exit_pos);
        world_add_renderable(world, exit_entity, exit_render);
        world_add_collider(world, exit_entity,
                           (Collider){.blocks_movement = true});
        world->room_exit = exit_entity;
        return;
    }
}

void setup_new_level(World *w) {
    w->player_data.floor++;
    // Reinitialize the floor
    floor_fill_void(w->floor);
    floor_generate_rooms(w->floor, ROOM_MIN_SIZE, ROOM_MAX_SIZE);
    floor_build_walls(w->floor);
    floor_connect_rooms(w->floor);

    // Reset fog of war
    for (int i = 0; i < w->floor->width * w->floor->height; i++) {
        w->floor->data[i].fog = false;
    }

    // Reset entity spatial index
    for (int y = 0; y < w->floor->height; y++) {
        for (int x = 0; x < w->floor->width; x++) {
            w->entity_at[y][x] = INVALID_ENTITY;
        }
    }

    // Remove all entities except player
    for (int e = 0; e < w->count; e++) {
        if (e == w->player)
            continue;

        world_remove_entity(w, e);
    }

    // Reset entity count if safe to do so
    if (w->player == 0) {
        w->count = 1;
    }

    // Reposition player in first room
    w->player_data.room_id = 0;
    Room *first_room = &w->floor->rooms[0];
    Position player_pos_start;

    // Temporarily remove player position to allow finding a spot
    w->has[w->player].position = false;
    if (!world_get_random_unoccupied_in_room(w, first_room,
                                             &player_pos_start)) {
        // Fallback to center if strangely full
        player_pos_start.x = first_room->x + first_room->w / 2;
        player_pos_start.y = first_room->y + first_room->h / 2;
    }

    // Ensure player has position component
    world_add_position(w, w->player, player_pos_start);

    // Reveal starting room
    floor_reveal_room(w->floor, 0);

    // Add new entities
    spawn_enemies_for_level(w);
    add_room_exit(w);
}
