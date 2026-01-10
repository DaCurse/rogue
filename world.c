#include "world.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "color.h"
#include "utils.h"

Entity world_create_entity(World *w) {
    Entity e = w->count++;
    w->entities[e] = e;
    w->has_position[e] = false;
    w->has_renderable[e] = false;
    return e;
}

void world_add_position(World *w, Entity e, Position pos) {
    w->positions[e] = pos;
    w->has_position[e] = true;
}

void world_add_renderable(World *w, Entity e, Renderable r) {
    w->renderables[e] = r;
    w->has_renderable[e] = true;
}

void world_add_combat_stats(World *w, Entity e, CombatStats cs) {
    w->combat_stats[e] = cs;
    w->has_combat_stats[e] = true;
}

void world_add_collider(World *w, Entity e, Collider c) {
    w->colliders[e] = c;
    w->has_collider[e] = true;
}

void world_add_move_intent(World *w, Entity e, MoveIntent mi) {
    w->move_intents[e] = mi;
    w->has_move_intent[e] = true;
}

void world_add_collision_event(World *w, Entity e, CollisionEvent ce) {
    w->collision_events[e] = ce;
    w->has_collision_event[e] = true;
}

void world_remove_entity(World *w, Entity e) {
    w->has_position[e] = false;
    w->has_renderable[e] = false;
    w->has_combat_stats[e] = false;
    w->has_collider[e] = false;
    w->has_move_intent[e] = false;
    w->has_collision_event[e] = false;
}

bool world_is_occupied(World *w, int x, int y) {
    for (int e = 0; e < w->count; e++) {
        if (!w->has_position[e])
            continue;
        if (w->positions[e].x == x && w->positions[e].y == y) {
            return true;
        }
    }
    return false;
}

int world_get_unoccupied_positions(World *w, Room *r, Position *out_arr,
                                   int max_len) {
    int count = 0;
    for (int y = r->y + 1; y < r->y + r->h - 1; y++) {
        for (int x = r->x + 1; x < r->x + r->w - 1; x++) {
            if (world_is_occupied(w, x, y))
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
        strncpy(w->log.messages[last_idx], buffer, LOG_MESSAGE_SIZE - 1);
        w->log.messages[last_idx][LOG_MESSAGE_SIZE - 1] = '\0';
        w->log.repeat_counts[last_idx] = 1;
    }

    w->log.turn_timestamp = w->player_data.turn_count;
}
