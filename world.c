#include "world.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "color.h"
#include "utils.h"

Entity world_create_entity(World *w) {
    Entity e = w->count++;
    w->entities[e] = e;
    memset(&w->has[e], 0, sizeof(ComponentFlags));
    return e;
}

static void world_remove_entity_from_spatial_index(World *w, Entity e) {
    if (!w->has[e].position)
        return;

    Position pos = w->positions[e];
    if (!in_bounds(w->floor, pos.x, pos.y))
        return;

    if (w->entity_at[pos.y][pos.x] == e)
        w->entity_at[pos.y][pos.x] = INVALID_ENTITY;
}

void world_move_entity(World *w, Entity e, int new_x, int new_y) {
    // If the position is already occupied, do nothing
    if (w->entity_at[new_y][new_x] != INVALID_ENTITY)
        return;

    assert(in_bounds(w->floor, new_x, new_y));

    world_remove_entity_from_spatial_index(w, e);

    w->positions[e].x = new_x;
    w->positions[e].y = new_y;

    w->entity_at[new_y][new_x] = e;
}

void world_add_position(World *w, Entity e, Position pos) {
    if (!in_bounds(w->floor, pos.x, pos.y)) {
        fprintf(stderr,
                "Error: Attempted to add position out of bounds (%d, %d)\n",
                pos.x, pos.y);
        abort();
    }

    world_remove_entity_from_spatial_index(w, e);

    w->positions[e] = pos;
    w->has[e].position = true;
    w->entity_at[pos.y][pos.x] = e;

    if (w->has[e].renderable) {
        hot_list_add_entity(w->render_list, &w->render_list_count, e);
    }
    if (w->has[e].combat_stats) {
        hot_list_add_entity(w->combatants, &w->combatants_count, e);
    }
    if (w->has[e].move_intent) {
        hot_list_add_entity(w->movers, &w->movers_count, e);
    }
}

void world_remove_position(World *w, Entity e) {
    world_remove_entity_from_spatial_index(w, e);
    w->has[e].position = false;
    hot_list_remove_entity(w->render_list, &w->render_list_count, e);
    hot_list_remove_entity(w->combatants, &w->combatants_count, e);
    hot_list_remove_entity(w->movers, &w->movers_count, e);
}

void world_add_renderable(World *w, Entity e, Renderable r) {
    w->renderables[e] = r;
    w->has[e].renderable = true;

    if (w->has[e].position) {
        hot_list_add_entity(w->render_list, &w->render_list_count, e);
    }
}

void world_add_combat_stats(World *w, Entity e, CombatStats cs) {
    w->combat_stats[e] = cs;
    w->has[e].combat_stats = true;

    if (w->has[e].position) {
        hot_list_add_entity(w->combatants, &w->combatants_count, e);
    }
}

void world_add_name(World *w, Entity e, Name n) {
    w->names[e] = n;
    w->has[e].name = true;
}

void world_add_collider(World *w, Entity e, Collider c) {
    w->colliders[e] = c;
    w->has[e].collider = true;
}

void world_add_ai(World *w, Entity e, AI ai) {
    w->ais[e] = ai;
    w->has[e].ai = true;
    hot_list_add_entity(w->ai_list, &w->ai_list_count, e);
}

void world_add_turn_delay(World *w, Entity e, TurnDelay td) {
    w->turn_delays[e] = td;
    w->has[e].turn_delay = true;
    hot_list_add_entity(w->turn_delay_list, &w->turn_delay_list_count, e);
}

void world_add_move_intent(World *w, Entity e, MoveIntent mi) {
    w->move_intents[e] = mi;
    w->has[e].move_intent = true;

    if (w->has[e].position) {
        hot_list_add_entity(w->movers, &w->movers_count, e);
    }
}

void world_add_collision_event(World *w, Entity e, CollisionEvent ce) {
    w->collision_events[e] = ce;
    w->has[e].collision_event = true;
    hot_list_add_entity(w->collisions, &w->collisions_count, e);
}

void world_remove_entity(World *w, Entity e) {
    // Remove from spatial index while position component is still valid
    world_remove_entity_from_spatial_index(w, e);
    // Now we can clear all components
    memset(&w->has[e], 0, sizeof(ComponentFlags));
    hot_list_remove_entity(w->render_list, &w->render_list_count, e);
    hot_list_remove_entity(w->combatants, &w->combatants_count, e);
    hot_list_remove_entity(w->ai_list, &w->ai_list_count, e);
    hot_list_remove_entity(w->turn_delay_list, &w->turn_delay_list_count, e);
    hot_list_remove_entity(w->collisions, &w->collisions_count, e);
}

int world_get_unoccupied_positions(World *w, Room *r, Position *out_arr,
                                   int max_len) {
    int count = 0;
    for (int y = r->y + 1; y < r->y + r->h - 1; y++) {
        for (int x = r->x + 1; x < r->x + r->w - 1; x++) {
            if (w->entity_at[y][x] != INVALID_ENTITY)
                continue;

            if (count < max_len) {
                out_arr[count++] = (Position){.x = x, .y = y};
            }
        }
    }

    return count;
}

bool world_get_random_unoccupied_in_room(World *w, Room *r, Position *out_pos) {
    Position candidates[ROOM_MAX_SIZE * ROOM_MAX_SIZE];
    int count = world_get_unoccupied_positions(w, r, candidates,
                                               ROOM_MAX_SIZE * ROOM_MAX_SIZE);

    if (count == 0)
        return false;

    int idx = random_int(0, count - 1);
    *out_pos = candidates[idx];

    return true;
}

void world_logf(World *w, const char *format, ...) {
    char buffer[LOG_MESSAGE_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Check repeats against the most recent message
    int last_idx = LOG_H - 1;
    if (strncmp(buffer, w->log.messages[last_idx], LOG_MESSAGE_SIZE) == 0) {
        w->log.repeat_counts[last_idx]++;
    } else {
        // Shift history up
        for (int i = 0; i < LOG_H - 1; i++) {
            memcpy(w->log.messages[i], w->log.messages[i + 1],
                   LOG_MESSAGE_SIZE);
            w->log.repeat_counts[i] = w->log.repeat_counts[i + 1];
        }

        // Add new message at the bottom
        snprintf(w->log.messages[last_idx], LOG_MESSAGE_SIZE, "%s", buffer);
        w->log.repeat_counts[last_idx] = 1;
    }

    w->log.turn_timestamp = w->player_data.turn_count;
}

void hot_list_add_entity(Entity *list, uint16_t *count, Entity e) {
    for (uint16_t i = 0; i < *count; i++) {
        if (list[i] == e)
            return;
    }
    list[(*count)++] = e;
}

void hot_list_remove_entity(Entity *list, uint16_t *count, Entity e) {
    for (uint16_t i = 0; i < *count; i++) {
        if (list[i] == e) {
            list[i] = list[--(*count)];
            return;
        }
    }
}

void hot_list_remove_index(Entity *list, uint16_t *count, size_t index) {
    if (index >= *count)
        return;

    list[index] = list[--(*count)];
}
