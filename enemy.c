#include "enemy.h"

#include <stdio.h>

#include "floor.h"
#include "utils.h"

// Spawning budget for first floor
#define BASE_THREAT_BUDGET (3)
// Additional points added to the budget for each deeper floor
#define THREAT_PER_FLOOR (3)
// Floor depth at which density increases to Tier 1
#define DENSITY_TIER_1_DEPTH (4)
// Floor depth at which density increases to Tier 2
#define DENSITY_TIER_2_DEPTH (8)
// Max enemies per room for floors < Tier 1
#define MAX_DENSITY_TIER_0 (1)
// Max enemies per room for floors >= Tier 1
#define MAX_DENSITY_TIER_1 (2)
// Max enemies per room for floors >= Tier 2
#define MAX_DENSITY_TIER_2 (3)

// Stat scaling factors
#define SCALE_HP_BASE (1.0f)
#define SCALE_HP_VAR (2.0f)
#define SCALE_ATK_BASE (0.2f)
#define SCALE_ATK_VAR (0.5f)
#define SCALE_DEF_VAR (0.25f)

// Maximum number of spawn slots
#define MAX_SPAWN_SLOTS (MAX_ROOMS * MAX_DENSITY_TIER_2)

static EnemyTemplate *select_enemy_for_depth(int floor_depth) {
    int total_weight = 0;
    int weights[sizeof(enemy_templates) / sizeof(EnemyTemplate)];
    int count = sizeof(enemy_templates) / sizeof(EnemyTemplate);

    // Calculate weights and total
    for (int i = 0; i < count; i++) {
        if (floor_depth < enemy_templates[i].min_depth) {
            weights[i] = 0;
            continue;
        }

        int weight = enemy_templates[i].base_weight +
                     (floor_depth - 1) * enemy_templates[i].depth_weight_mod;
        weights[i] = MAX(1, weight); // Ensure at least 1 if valid depth
        total_weight += weights[i];
    }

    // Should not happen if roster is good, but fallback safely
    if (total_weight == 0)
        return &enemy_templates[0];

    // Weighted selection
    int roll = random_int(0, total_weight - 1);
    int current_sum = 0;

    for (int i = 0; i < count; i++) {
        current_sum += weights[i];
        if (roll < current_sum) {
            return &enemy_templates[i];
        }
    }

    return &enemy_templates[0];
}

static void spawn_single_enemy_at(World *world, Position pos, int floor_depth) {
    EnemyTemplate *tmpl = select_enemy_for_depth(floor_depth);

    // Calculate Scaled Stats
    float scale = tmpl->scale_factor;
    float depth_factor = (float)floor_depth;

    // HP Bonus: Bigger scaling
    int hp_var = (int)(depth_factor * scale * SCALE_HP_VAR);
    int hp_bonus = (int)(depth_factor * scale * SCALE_HP_BASE) +
                   random_int(0, MAX(1, hp_var));
    int final_hp = tmpl->base_hp + hp_bonus;

    // Atk Bonus: Moderate scaling
    int atk_var = (int)(depth_factor * scale * SCALE_ATK_VAR);
    int atk_bonus = (int)(depth_factor * scale * SCALE_ATK_BASE) +
                    random_int(0, MAX(1, atk_var));
    int final_atk = tmpl->base_attack + atk_bonus;

    // Def Bonus: Small scaling (capped/slow)
    int def_var = (int)(depth_factor * scale * SCALE_DEF_VAR);
    int def_bonus = random_int(0, MAX(1, def_var));
    int final_def = tmpl->base_defense + def_bonus;

    Entity enemy = world_create_entity(world);
    Renderable enemy_render = {.glyph = tmpl->glyph, .color_pair = tmpl->color};
    CombatStats combat_stats = {
        .hp = final_hp,
        .max_hp = final_hp,
        .attack = final_atk,
        .defense = final_def,
    };
    snprintf(combat_stats.name, sizeof(combat_stats.name), "%s", tmpl->name);

    world_add_position(world, enemy, pos);
    world_add_renderable(world, enemy, enemy_render);
    world_add_combat_stats(world, enemy, combat_stats);
    world_add_collider(world, enemy, (Collider){.blocks_movement = true});
    world_add_ai(world, enemy, tmpl->ai);
    world_add_turn_delay(world, enemy, tmpl->turn_delay);
}

static int get_max_enemies_per_room(int floor_depth) {
    if (floor_depth >= DENSITY_TIER_2_DEPTH)
        return MAX_DENSITY_TIER_2;
    if (floor_depth >= DENSITY_TIER_1_DEPTH)
        return MAX_DENSITY_TIER_1;
    return MAX_DENSITY_TIER_0;
}

void spawn_enemies_for_level(World *world) {
    int floor_depth = world->player_data.floor;
    int threat_budget =
        BASE_THREAT_BUDGET + (THREAT_PER_FLOOR * (floor_depth - 1));
    int max_per_room = get_max_enemies_per_room(floor_depth);

    // Cache available positions for each room
    Position room_free_tiles[MAX_ROOMS][ROOM_MAX_SIZE * ROOM_MAX_SIZE];
    int room_free_counts[MAX_ROOMS];
    int room_next_idx[MAX_ROOMS];

    for (int i = 0; i < world->floor->room_count; i++) {
        room_next_idx[i] = 0;
        int count = world_get_unoccupied_positions(
            world, &world->floor->rooms[i], room_free_tiles[i],
            ROOM_MAX_SIZE * ROOM_MAX_SIZE);
        shuffle_array(room_free_tiles[i], count, sizeof(Position));
        room_free_counts[i] = count;
    }

    // Create a shuffle bag of spawn slots. Adding each room 'max_per_room'
    // times ensures we match the density limit without needing rejection
    // sampling.
    int room_slots[MAX_SPAWN_SLOTS];
    int slot_count = 0;

    for (int i = 0; i < world->floor->room_count; i++) {
        for (int k = 0; k < max_per_room; k++) {
            if (slot_count < MAX_SPAWN_SLOTS) {
                room_slots[slot_count++] = i;
            }
        }
    }

    // Shuffle the slots to randomize order
    shuffle_array(room_slots, slot_count, sizeof(int));

    // Iterate through slots and fill with enemies until budget runs out
    for (int i = 0; i < slot_count && threat_budget > 0; i++) {

        int room_idx = room_slots[i];
        if (room_next_idx[room_idx] >= room_free_counts[room_idx]) {
            continue;
        }

        EnemyTemplate *tmpl = select_enemy_for_depth(floor_depth);

        if (tmpl->threat_cost > threat_budget) {
            continue;
        }

        // Get next valid position
        Position pos = room_free_tiles[room_idx][room_next_idx[room_idx]++];
        spawn_single_enemy_at(world, pos, floor_depth);
        threat_budget -= tmpl->threat_cost;
    }
}
