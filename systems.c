#include <ncurses.h>
#include <stdio.h>

#include "color.h"
#include "game.h"
#include "systems.h"
#include "utils.h"

static Renderable tile_glyph(Floor *f, int x, int y) {
    switch (tile_at(f, x, y)) {
    case TILE_VOID:
        return (Renderable){.glyph = ' ', .color_pair = COLOR_PAIR_VOID};
    case TILE_FLOOR:
        return (Renderable){.glyph = '.', .color_pair = COLOR_PAIR_FLOOR};
    case TILE_ROAD:
        return (Renderable){.glyph = '#', .color_pair = COLOR_PAIR_ROAD};
    case TILE_WALL:
        switch (f->walls[x + f->width * y]) {
        case WALL_VERTICAL:
            return (Renderable){.glyph = '|', .color_pair = COLOR_PAIR_WALL};
        case WALL_HORIZONTAL:
        case WALL_CORNER:
            return (Renderable){.glyph = '-', .color_pair = COLOR_PAIR_WALL};
        default:
            return (Renderable){.glyph = '?', .color_pair = COLOR_PAIR_UNKNOWN};
        }
    default:
        return (Renderable){.glyph = '?', .color_pair = COLOR_PAIR_UNKNOWN};
    }
}

void system_render_map(World *w) {
    werase(w->map.win);

    // Render floor
    for (int y = 0; y < w->floor->height; y++) {
        for (int x = 0; x < w->floor->width; x++) {
#ifndef DEBUG_REVEAL_MAP
            if (!w->floor->fog_of_war[x + w->floor->width * y])
                continue;
#endif

            Renderable r = tile_glyph(w->floor, x, y);
            mvwaddch(w->map.win, y, x, r.glyph | COLOR_PAIR(r.color_pair));
        }
    }

    // Render entities
    for (int e = 0; e < w->count; e++) {
        if (!(w->has_position[e] && w->has_renderable[e]))
            continue;

        Position *p = &w->positions[e];

#ifndef DEBUG_REVEAL_MAP
        if (w->floor->fog_of_war[p->x + w->floor->width * p->y] == false)
            continue;
#endif

        Renderable *r = &w->renderables[e];

        mvwaddch(w->map.win, p->y, p->x, r->glyph | COLOR_PAIR(r->color_pair));
    }

    wbkgd(w->map.win, COLOR_PAIR(COLOR_PAIR_VOID));
    wrefresh(w->map.win);
}

void system_render_logs(World *w) {
    werase(w->log_window.win);

    if (w->log_message[0]) {
        if (w->log_repeat_count > 1) {
            char suffix[32];
            snprintf(suffix, sizeof(suffix), " (x%d)", w->log_repeat_count);

            int max_msg_len = LOG_MESSAGE_SIZE - strlen(suffix) - 1;
            mvwprintw(w->log_window.win, 0, 0, " %.*s%s", max_msg_len,
                      w->log_message, suffix);
        } else {
            mvwprintw(w->log_window.win, 0, 0, " %s", w->log_message);
        }
    }

    wrefresh(w->log_window.win);
}

void system_render_status_bar(World *w) {
    werase(w->status_bar.win);

    CombatStats *cs = &w->combat_stats[w->player];

    wbkgd(w->status_bar.win, COLOR_PAIR(COLOR_PAIR_STATUS));
    mvwprintw(w->status_bar.win, 0, 1, "Level: 1 Floor: %d HP: %d/%d Seed: %u",
              w->player_data.floor, cs->hp, cs->max_hp, w->seed);

    wrefresh(w->status_bar.win);
}

void system_player_input(World *w, char key) {
    int dx = 0, dy = 0;

    switch (tolower(key)) {
    case 'w':
        dy = -1;
        break;
    case 's':
        dy = 1;
        break;
    case 'a':
        dx = -1;
        break;
    case 'd':
        dx = 1;
        break;
    default:
        return;
    }

    world_add_move_intent(w, w->player, (MoveIntent){.dx = dx, .dy = dy});
}

static CollisionResult check_collision(World *w, Floor *f, Entity e1,
                                       int target_x, int target_y) {
    // Map collision
    Tile t = tile_at(f, target_x, target_y);
    if (t == TILE_WALL || t == TILE_VOID) {
        return (CollisionResult){.type = COLLISION_MAP,
                                 .entity = INVALID_ENTITY};
    }

    // Entity collision
    for (int e2 = 0; e2 < w->count; e2++) {
        if (e1 == e2)
            continue;
        if (!(w->has_position[e2] && w->has_collider[e2]))
            continue;

        Position *p2 = &w->positions[e2];
        Collider *c2 = &w->colliders[e2];

        if (c2->blocks_movement && p2->x == target_x && p2->y == target_y) {
            CollisionResult result = {.type = COLLISION_ENTITY, .entity = e2};
            return result;
        }
    }

    return (CollisionResult){.type = COLLISION_NONE, .entity = INVALID_ENTITY};
}

static void handle_player_movement(World *w, Position *p) {
    Tile current_tile = tile_at(w->floor, p->x, p->y);
    if (current_tile == TILE_ROAD) {
        // On road: reveal only immediate surroundings (radius 1)
        floor_reveal_area(w->floor, p->x, p->y, 0);
        w->player_data.room_id = -1;
    } else if (current_tile == TILE_FLOOR) {
        // In room: reveal the entire room
        int room_idx = floor_find_room(w->floor, p->x, p->y);
        if (room_idx >= 0) {
            floor_reveal_room(w->floor, room_idx);
            w->player_data.room_id = room_idx;
        }
    }
}

void system_movement(World *w) {
    for (int e = 0; e < w->count; e++) {
        if (!(w->has_position[e] && w->has_move_intent[e]))
            continue;

        Position *p = &w->positions[e];
        MoveIntent *mi = &w->move_intents[e];

        int new_x = p->x + mi->dx;
        int new_y = p->y + mi->dy;

        CollisionResult cr = check_collision(w, w->floor, e, new_x, new_y);
        switch (cr.type) {

        case COLLISION_ENTITY:
            CollisionEvent ce = {.target = cr.entity};
            world_add_collision_event(w, e, ce);
            break;

        case COLLISION_NONE:
            p->x = new_x;
            p->y = new_y;

            if (e == w->player) {
                handle_player_movement(w, p);
            }
            break;

        case COLLISION_MAP:
        default:
            break;
        }

        w->has_move_intent[e] = false;
    }
}

static void log_combat_event(World *w, const char *attacker,
                             const char *defender, int damage_dealt,
                             int damage_taken) {
    char attack_msg[128];
    if (damage_dealt > 0) {
        snprintf(attack_msg, sizeof(attack_msg),
                 "The %s strikes the %s for %d damage!", attacker, defender,
                 damage_dealt);
    } else {
        snprintf(attack_msg, sizeof(attack_msg),
                 "The %s strikes the %s but the attack is blocked!", attacker,
                 defender);
    }

    char counter_msg[128];
    if (damage_taken > 0) {
        snprintf(counter_msg, sizeof(counter_msg),
                 "The %s counters, dealing %d damage.", defender, damage_taken);
    } else {
        snprintf(counter_msg, sizeof(counter_msg),
                 "The %s counters but deals no damage.", defender);
    }

    world_logf(w, "%s %s", attack_msg, counter_msg);
}

void system_combat(World *w) {
    for (int e = 0; e < w->count; e++) {
        if (!w->has_collision_event[e])
            continue;

        CollisionEvent *ce = &w->collision_events[e];
        Entity attacker = e;
        Entity defender = ce->target;

        // Check if either entity is the player and both have combat stats
        if (!(attacker == w->player || defender == w->player))
            continue;
        if (!(w->has_combat_stats[attacker] && w->has_combat_stats[defender]))
            continue;

        CombatStats *attacker_stats = &w->combat_stats[attacker];
        CombatStats *defender_stats = &w->combat_stats[defender];

        int damage_to_defender =
            MAX(0, attacker_stats->attack - defender_stats->defense);
        int damage_to_attacker =
            MAX(0, defender_stats->attack - attacker_stats->defense);

        defender_stats->hp -= damage_to_defender;
        attacker_stats->hp -= damage_to_attacker;

        log_combat_event(w, attacker_stats->name, defender_stats->name,
                         damage_to_defender, damage_to_attacker);

        w->has_collision_event[e] = false;
    }
}

void system_exit_room(World *w) {
    if (!w->has_collision_event[w->player])
        return;

    CollisionEvent *ce = &w->collision_events[w->player];
    if (ce->target == w->room_exit) {
        // Player collided with room exit
        w->has_collision_event[w->player] = false;
        world_logf(w, "You descend deeper into the dungeon.");

        setup_new_level(w);
    }
}

void system_death(World *w) {
    for (int e = 0; e < w->count; e++) {
        if (!w->has_combat_stats[e])
            continue;

        CombatStats *cs = &w->combat_stats[e];
        if (cs->hp <= 0) {
            world_logf(w, "The %s dies.", cs->name);

            // Remove all components (entity cleanup)
            world_remove_entity(w, e);
        }
    }
}