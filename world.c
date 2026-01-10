#include "world.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "color.h"
#include "utils.h"

static void add_to_list(Entity *list, int *count, Entity e) {
    for (int i = 0; i < *count; i++) {
        if (list[i] == e)
            return;
    }
    list[(*count)++] = e;
}

static void remove_from_list(Entity *list, int *count, Entity e) {
    for (int i = 0; i < *count; i++) {
        if (list[i] == e) {
            list[i] = list[--(*count)];
            return;
        }
    }
}

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
    int x = pos.x, y = pos.y;
    if (in_bounds(w->floor, x, y) && w->entity_at[y][x] == e) {
        w->entity_at[y][x] = INVALID_ENTITY;
    }
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
        add_to_list(w->render_list, &w->render_list_count, e);
    }
    if (w->has[e].combat_stats) {
        add_to_list(w->combatants, &w->combatants_count, e);
    }
    if (w->has[e].move_intent) {
        add_to_list(w->movers, &w->movers_count, e);
    }
}

void world_add_renderable(World *w, Entity e, Renderable r) {
    w->renderables[e] = r;
    w->has[e].renderable = true;

    if (w->has[e].position) {
        add_to_list(w->render_list, &w->render_list_count, e);
    }
}

void world_add_combat_stats(World *w, Entity e, CombatStats cs) {
    w->combat_stats[e] = cs;
    w->has[e].combat_stats = true;

    if (w->has[e].position) {
        add_to_list(w->combatants, &w->combatants_count, e);
    }
}

void world_add_collider(World *w, Entity e, Collider c) {
    w->colliders[e] = c;
    w->has[e].collider = true;
}

void world_add_move_intent(World *w, Entity e, MoveIntent mi) {
    w->move_intents[e] = mi;
    w->has[e].move_intent = true;

    if (w->has[e].position) {
        add_to_list(w->movers, &w->movers_count, e);
    }
}

void world_add_collision_event(World *w, Entity e, CollisionEvent ce) {
    w->collision_events[e] = ce;
    w->has[e].collision_event = true;
    add_to_list(w->collisions, &w->collisions_count, e);
}

void world_remove_entity(World *w, Entity e) {
    memset(&w->has[e], 0, sizeof(ComponentFlags));
    remove_from_list(w->render_list, &w->render_list_count, e);
    remove_from_list(w->combatants, &w->combatants_count, e);
    world_remove_entity_from_spatial_index(w, e);
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
