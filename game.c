#include "game.h"
#include "config.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

void create_player(World *world) {
    Entity player = world_create_entity(world);
    world->player = player;
    world->player_data = (Player){
        .room_id = 0,
    };

    Renderable player_render = {.glyph = '@', .color_pair = COLOR_PAIR_PLAYER};
    CombatStats combat_stats = {
        .name = "Player",
        .hp = 100,
        .max_hp = 100,
        .attack = 10,
        .defense = 0,
    };

    // Position will be set in setup_new_level
    world_add_renderable(world, player, player_render);
    world_add_combat_stats(world, player, combat_stats);
}

void create_enemy(World *world) {
    Room *room =
        floor_random_room_excl(world->floor, world->player_data.room_id);
    if (room == NULL) {
        // This might happen if there's only 1 room (the player's room)
        return;
    }

    Entity enemy = world_create_entity(world);
    Position enemy_pos = {
        .x = room->x + random_int(1, room->w - 2),
        .y = room->y + random_int(1, room->h - 2),
    };
    Renderable enemy_render = {.glyph = 'E', .color_pair = COLOR_PAIR_ENEMY};
    CombatStats combat_stats = {
        .name = "Enemy",
        .hp = 20,
        .max_hp = 20,
        .attack = 5,
        .defense = 2,
    };
    world_add_position(world, enemy, enemy_pos);
    world_add_renderable(world, enemy, enemy_render);
    world_add_combat_stats(world, enemy, combat_stats);
    world_add_collider(world, enemy, (Collider){.blocks_movement = true});
}

void add_room_exit(World *world) {
    Room *room =
        floor_random_room_excl(world->floor, world->player_data.room_id);
    if (room == NULL) {
        return;
    }

    Entity exit = world_create_entity(world);
    Position exit_pos = {
        .x = room->x + room->w - 2,
        .y = room->y + room->h - 2,
    };
    Renderable exit_render = {.glyph = 'H', .color_pair = COLOR_PAIR_EXIT};
    world_add_position(world, exit, exit_pos);
    world_add_renderable(world, exit, exit_render);
    world_add_collider(world, exit, (Collider){.blocks_movement = true});

    world->room_exit = exit;
}

void setup_new_level(World *w) {
    // Reinitialize the floor
    floor_fill_void(w->floor);
    floor_generate_rooms(w->floor, ROOM_MIN_SIZE, ROOM_MAX_SIZE);
    floor_build_walls(w->floor);
    floor_connect_rooms(w->floor);

    // Reset fog of war
    for (int i = 0; i < w->floor->width * w->floor->height; i++) {
        w->floor->fog_of_war[i] = false;
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
    Position player_pos_start = {
        .x = first_room->x + random_int(1, first_room->w - 2),
        .y = first_room->y + random_int(1, first_room->h - 2),
    };

    // Ensure player has position component 
    world_add_position(w, w->player, player_pos_start);

    // Reveal starting room
    floor_reveal_room(w->floor, 0);

    // Add new entities
    create_enemy(w);
    add_room_exit(w);
}
