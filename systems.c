#include <ncurses.h>

#include "color.h"
#include "systems.h"

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

    box(w->log_window.win, 0, 0);
    mvwprintw(w->log_window.win, 0, 2, " Journal ");
    int max_logs = getmaxy(w->log_window.win) - 2;
    int start_idx = (w->log_count > max_logs) ? w->log_count - max_logs : 0;

    for (int i = 0; i < max_logs && start_idx + i < w->log_count; i++) {
        mvwprintw(w->log_window.win, i + 1, 1, "%s",
                  w->log_entries[start_idx + i].log_message);
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
                // Reveal less area if on road
                int light_radius = tile_at(w->floor, p->x, p->y) == TILE_ROAD
                                       ? 1
                                       : w->player_data.light_radius;
                floor_reveal_area(w->floor, p->x, p->y, light_radius);
            }
            break;

        default:
            break;
        }

        w->has_move_intent[e] = false;
    }
}

void system_combat(World *w) {
    for (int e = 0; e < w->count; e++) {
        if (!(w->has_combat_intent[e] && w->has_combat_stats[e]))
            continue;

        CombatIntent *ci = &w->combat_intents[e];
        CombatStats *attacker_stats = &w->combat_stats[ci->attacker];
        CombatStats *defender_stats = &w->combat_stats[ci->defender];

        int damage_to_defender =
            attacker_stats->attack > defender_stats->defense
                ? attacker_stats->attack - defender_stats->defense
                : 1;
        int damage_to_attacker =
            defender_stats->attack > attacker_stats->defense
                ? defender_stats->attack - attacker_stats->defense
                : 1;

        defender_stats->hp -= damage_to_defender;
        attacker_stats->hp -= damage_to_attacker;

        // TODO: Rework logging system
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "%s attacked %s for %d damage\n",
                 attacker_stats->name, defender_stats->name,
                 damage_to_defender);
        world_add_log_entry(w, log_msg);
        snprintf(log_msg, sizeof(log_msg), "%s retaliated %s for %d damage\n",
                 defender_stats->name, attacker_stats->name,
                 damage_to_attacker);
        world_add_log_entry(w, log_msg);

        w->has_combat_intent[e] = false;
    }
}

void system_death(World *w) {
    for (int e = 0; e < w->count; e++) {
        if (!w->has_combat_stats[e])
            continue;

        CombatStats *cs = &w->combat_stats[e];
        if (cs->hp <= 0) {
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "%s has been defeated!\n",
                     cs->name);
            world_add_log_entry(w, log_msg);

            // Remove all components (entity cleanup)
            w->has_position[e] = false;
            w->has_renderable[e] = false;
            w->has_combat_stats[e] = false;
            w->has_collider[e] = false;
        }
    }
}