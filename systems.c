#include <ncurses.h>
#include <stdio.h>

#include "color.h"
#include "systems.h"
#include "utils.h"

void system_render_map(World *w) {
    werase(w->map.win);

    // Render floor
    for (int y = 0; y < w->floor->height; y++) {
        for (int x = 0; x < w->floor->width; x++) {
            if (!w->floor->fog_of_war[x + w->floor->width * y])
                continue;

            Renderable r = tile_glyph(w->floor, x, y);
            mvwaddch(w->map.win, y, x, r.glyph | COLOR_PAIR(r.color_pair));
        }
    }

    // Render entities
    for (int e = 0; e < w->count; e++) {
        if (!(w->has_position[e] && w->has_renderable[e]))
            continue;

        Position *p = &w->positions[e];

        if (w->floor->fog_of_war[p->x + w->floor->width * p->y] == false)
            continue;

        Renderable *r = &w->renderables[e];

        mvwaddch(w->map.win, p->y, p->x, r->glyph | COLOR_PAIR(r->color_pair));
    }

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

    wbkgd(w->status_bar.win, COLOR_PAIR(COLOR_STATUS));
    mvwprintw(w->status_bar.win, 0, 1, "Level: 1 Floor: 1 HP: %d/%d Seed: %u",
              cs->hp, cs->max_hp, w->seed);

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

CollisionResult check_collision(World *w, Floor *f, Entity e1, int target_x,
                                int target_y) {
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
        case COLLISION_MAP:
            // Blocked by map
            break;
        case COLLISION_ENTITY:
            // Handle entity collision

            // If both entities have combat stats and one is a player, initiate
            // combat
            if (w->has_combat_stats[e] && w->has_combat_stats[cr.entity] &&
                (e == w->player || cr.entity == w->player)) {
                CombatIntent ci = {.attacker = e, .defender = cr.entity};
                world_add_combat_intent(w, e, ci);
            }
            break;
        case COLLISION_NONE:
            // No collision, proceed with movement
            p->x = new_x;
            p->y = new_y;

            if (e == w->player) {
                Tile current_tile = tile_at(w->floor, p->x, p->y);
                if (current_tile == TILE_ROAD) {
                    // On road: reveal only immediate surroundings (radius 1)
                    floor_reveal_area(w->floor, p->x, p->y, 1);
                } else if (current_tile == TILE_FLOOR) {
                    // In room: reveal the entire room
                    int room_idx = floor_find_room(w->floor, p->x, p->y);
                    if (room_idx >= 0) {
                        floor_reveal_room(w->floor, room_idx);
                    }
                }
            }
            break;

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
        if (!(w->has_combat_intent[e] && w->has_combat_stats[e]))
            continue;

        CombatIntent *ci = &w->combat_intents[e];
        CombatStats *attacker_stats = &w->combat_stats[ci->attacker];
        CombatStats *defender_stats = &w->combat_stats[ci->defender];

        int damage_to_defender =
            MAX(0, attacker_stats->attack - defender_stats->defense);
        int damage_to_attacker =
            MAX(0, defender_stats->attack - attacker_stats->defense);

        defender_stats->hp -= damage_to_defender;
        attacker_stats->hp -= damage_to_attacker;

        log_combat_event(w, attacker_stats->name, defender_stats->name,
                         damage_to_defender, damage_to_attacker);

        w->has_combat_intent[e] = false;
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
            w->has_position[e] = false;
            w->has_renderable[e] = false;
            w->has_combat_stats[e] = false;
            w->has_collider[e] = false;
        }
    }
}