#include "world.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "color.h"

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

void world_logf(World *w, const char *format, ...) {
    char buffer[LOG_MESSAGE_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (strncmp(buffer, w->log_message, LOG_MESSAGE_SIZE) == 0) {
        w->log_repeat_count++;
    } else {
        w->log_repeat_count = 1;
        strncpy(w->log_message, buffer, LOG_MESSAGE_SIZE - 1);
        w->log_message[LOG_MESSAGE_SIZE - 1] = '\0';
    }
}
