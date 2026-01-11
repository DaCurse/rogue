#include "systems.h"

#include <ctype.h>
#include <ncurses.h>
#include <stdio.h>

#include "color.h"
#include "game.h"
#include "utils.h"

#define MESSAGE_BOREDOM_THRESHOLD (15)

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
            if (!w->floor->data[x + w->floor->width * y].fog)
                continue;
#endif

            Renderable r = tile_glyph(w->floor, x, y);
            mvwaddch(w->map.win, y, x,
                     (chtype)r.glyph | COLOR_PAIR(r.color_pair));
        }
    }

    // Render entities
    FOR_EACH_ACTIVE(w, render_list, e) {
        Position *p = &w->positions[e];

#ifndef DEBUG_REVEAL_MAP
        if (w->floor->data[p->x + w->floor->width * p->y].fog == false)
            continue;
#endif

        Renderable *r = &w->renderables[e];

        mvwaddch(w->map.win, p->y, p->x,
                 (chtype)r->glyph | COLOR_PAIR(r->color_pair));
    }

    wbkgd(w->map.win, COLOR_PAIR(COLOR_PAIR_VOID));
    wrefresh(w->map.win);
}

void system_render_logs(World *w) {
    // Message boredom: clear message if too old
    if (w->player_data.turn_count - w->log.turn_timestamp >
        MESSAGE_BOREDOM_THRESHOLD) {
        for (int i = 0; i < LOG_H; i++) {
            w->log.messages[i][0] = '\0';
            w->log.repeat_counts[i] = 0;
        }
    }

    werase(w->log_window.win);

    for (int i = 0; i < LOG_H; i++) {
        if (w->log.messages[i][0] == '\0')
            continue;

        if (w->log.repeat_counts[i] > 1) {
            char suffix[32];
            snprintf(suffix, sizeof(suffix), " (x%d)", w->log.repeat_counts[i]);

            int max_msg_len =
                MAX(0, (int)LOG_MESSAGE_SIZE - (int)strlen(suffix) - 1);
            mvwprintw(w->log_window.win, i, 0, " %.*s%s", max_msg_len,
                      w->log.messages[i], suffix);
        } else {
            mvwprintw(w->log_window.win, i, 0, " %s", w->log.messages[i]);
        }
    }

    wrefresh(w->log_window.win);
}

void system_render_status_bar(World *w) {
    werase(w->status_bar.win);

    CombatStats *cs = &w->combat_stats[w->player];

    wbkgd(w->status_bar.win, COLOR_PAIR(COLOR_PAIR_STATUS));
    mvwprintw(w->status_bar.win, 0, 1,
              "%s, Level 1 adventurer | Floor: %d HP: %d/%d Seed: %u",
              w->combat_stats[w->player].name, w->player_data.floor, cs->hp,
              cs->max_hp, w->seed);

    wrefresh(w->status_bar.win);
}

void system_player_input(World *w, int ch) {
    int dx = 0, dy = 0;

    switch (tolower(ch)) {
    case KEY_UP:
    case 'w':
        dy = -1;
        break;
    case KEY_DOWN:
    case 's':
        dy = 1;
        break;
    case KEY_LEFT:
    case 'a':
        dx = -1;
        break;
    case KEY_RIGHT:
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

    // Position is guaranteed to be in bounds because of tile_at call

    // Entity collision
    Entity e2 = w->entity_at[target_y][target_x];
    if (e2 != INVALID_ENTITY && e2 != e1 && w->has[e2].collider &&
        w->colliders[e2].blocks_movement) {
        return (CollisionResult){.type = COLLISION_ENTITY, .entity = e2};
    }

    return (CollisionResult){.type = COLLISION_NONE, .entity = INVALID_ENTITY};
}

static void handle_player_movement(World *w, Position *p) {
    Tile current_tile = tile_at(w->floor, p->x, p->y);
    if (current_tile == TILE_ROAD) {
        // On road: reveal immediate road tiles and nearby floor tiles
        floor_reveal_filtered(w->floor, p->x, p->y, 2, (1 << TILE_ROAD));
        floor_reveal_area(w->floor, p->x, p->y, 1);
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
    FOR_EACH_ACTIVE(w, movers, e) {
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
            world_move_entity(w, e, new_x, new_y);

            if (e == w->player) {
                handle_player_movement(w, p);
            }
            break;

        case COLLISION_MAP:
        default:
            break;
        }

        w->has[e].move_intent = false;
    }

    // Clear hot movers list
    w->movers_count = 0;
}

static void log_combat_event(World *w, const char *attacker,
                             const char *defender, int damage_dealt,
                             int damage_taken) {
    char attack_msg[LOG_MESSAGE_SIZE / 2];
    if (damage_dealt > 0) {
        snprintf(attack_msg, sizeof(attack_msg), "%s strikes %s for %d damage!",
                 attacker, defender, damage_dealt);
    } else {
        snprintf(attack_msg, sizeof(attack_msg),
                 "%s strikes %s but the attack is blocked!", attacker,
                 defender);
    }

    char counter_msg[LOG_MESSAGE_SIZE / 2];
    if (damage_taken > 0) {
        snprintf(counter_msg, sizeof(counter_msg),
                 "%s counters, dealing %d damage.", defender, damage_taken);
    } else {
        snprintf(counter_msg, sizeof(counter_msg),
                 "%s counters but deals no damage.", defender);
    }

    world_logf(w, "%s %s", attack_msg, counter_msg);
}

void system_combat(World *w) {
    FOR_EACH_ACTIVE(w, collisions, e) {
        CollisionEvent *ce = &w->collision_events[e];
        Entity attacker = e;
        Entity defender = ce->target;

        // Check if either entity is the player and both have combat stats
        if (!(attacker == w->player || defender == w->player))
            continue;
        if (!(w->has[attacker].combat_stats && w->has[defender].combat_stats))
            continue;

        CombatStats *attacker_stats = &w->combat_stats[attacker];
        CombatStats *defender_stats = &w->combat_stats[defender];

        int damage_to_defender =
            MAX(0, attacker_stats->attack - defender_stats->defense);
        int damage_to_attacker =
            MAX(0, defender_stats->attack - attacker_stats->defense);

        defender_stats->hp = MAX(0, defender_stats->hp - damage_to_defender);
        attacker_stats->hp = MAX(0, attacker_stats->hp - damage_to_attacker);

        log_combat_event(w, attacker_stats->name, defender_stats->name,
                         damage_to_defender, damage_to_attacker);

        w->has[e].collision_event = false;
    }

    // Clear hot collisions list
    w->collisions_count = 0;
}

void system_exit_room(World *w) {
    if (!w->has[w->player].collision_event)
        return;

    CollisionEvent *ce = &w->collision_events[w->player];
    if (ce->target == w->room_exit) {
        // Player collided with room exit
        w->has[w->player].collision_event = false;
        world_logf(w, "You descend deeper into the dungeon.");

        setup_new_level(w);
    }
}

void system_death(World *w) {
    FOR_EACH_ACTIVE_REVERSE(w, combatants, e) {
        CombatStats *cs = &w->combat_stats[e];
        if (cs->hp <= 0) {
            world_logf(w, "%s dies.", cs->name);

            if (e == w->player) {
                w->player_data.game_over = true;
                world_logf(w, "Game over! Press any key to continue...");
            }

            // Remove all components (entity cleanup)
            world_remove_entity(w, e);
        }
    }
}
