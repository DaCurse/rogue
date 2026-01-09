#include "ecs.h"

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

void world_add_combat_intent(World *w, Entity e, CombatIntent ci) {
    w->combat_intents[e] = ci;
    w->has_combat_intent[e] = true;
}

void world_add_log_entry(World *w, const char *message) {
    if (w->log_count < MAX_ENTITIES) {
        strncpy(w->log_entries[w->log_count].log_message, message, 255);
        w->log_entries[w->log_count].log_message[255] = '\0';
        w->log_count++;
    } else {
        // Shift all entries up by one (remove oldest)
        for (int i = 0; i < MAX_ENTITIES - 1; i++) {
            w->log_entries[i] = w->log_entries[i + 1];
        }
        // Add new entry at the end
        strncpy(w->log_entries[MAX_ENTITIES - 1].log_message, message, 255);
        w->log_entries[MAX_ENTITIES - 1].log_message[255] = '\0';
    }
}

Renderable tile_glyph(Floor *f, int x, int y) {
    switch (tile_at(f, x, y)) {
    case TILE_VOID:
        return (Renderable){.glyph = ' ', .color_pair = COLOR_VOID};
    case TILE_FLOOR:
        return (Renderable){.glyph = '.', .color_pair = COLOR_FLOOR};
    case TILE_ROAD:
        return (Renderable){.glyph = ' ', .color_pair = COLOR_ROAD};
    case TILE_WALL:
        switch (f->walls[x + f->width * y]) {
        case WALL_VERTICAL:
            return (Renderable){.glyph = '|', .color_pair = COLOR_WALL};
        case WALL_HORIZONTAL:
            return (Renderable){.glyph = '_', .color_pair = COLOR_WALL};
        case WALL_CORNER:
            return (Renderable){.glyph = '+', .color_pair = COLOR_WALL};
        default:
            return (Renderable){.glyph = '?', .color_pair = COLOR_UNKNOWN};
        }
    default:
        return (Renderable){.glyph = '?', .color_pair = COLOR_UNKNOWN};
    }
}
