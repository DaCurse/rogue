#ifndef ENEMY_H
#define ENEMY_H

#include "world.h"

typedef struct {
    char *name;
    char glyph;
    ColorPair color;

    // Base stats
    int16_t base_hp;
    int16_t base_attack;
    int16_t base_defense;

    // Scaling
    float scale_factor;

    // Spawning logic
    uint8_t threat_cost;
    uint8_t min_depth;
    int16_t base_weight;
    int16_t depth_weight_mod;

    // AI configuration
    AI ai;
    TurnDelay turn_delay;
} EnemyTemplate;

void spawn_enemies_for_level(World *world);

static EnemyTemplate enemy_templates[] = {
    // Rat: Low HP, Low Atk, No Def. Common early game.
    {
        .name = "Rat",
        .glyph = 'r',
        .color = COLOR_PAIR_ENEMY_RAT,
        .base_hp = 10,
        .base_attack = 3,
        .base_defense = 0,
        .scale_factor = 0.5f,
        .threat_cost = 1,
        .min_depth = 1,
        .base_weight = 100,
        .depth_weight_mod = -2,
        .ai = (AI){.aware = false, .detection_radius = 4},
        .turn_delay = (TurnDelay){.delay = 2, .timer = 0},
    },

    // Goblin: Med HP, Med Atk, Low Def. Baseline enemy.
    {
        .name = "Goblin",
        .glyph = 'g',
        .color = COLOR_PAIR_ENEMY_GOBLIN,
        .base_hp = 20,
        .base_attack = 5,
        .base_defense = 1,
        .scale_factor = 1.0f,
        .threat_cost = 3,
        .min_depth = 1,
        .base_weight = 60,
        .depth_weight_mod = 1,
        .ai = (AI){.aware = false, .detection_radius = 5},
        .turn_delay = (TurnDelay){.delay = 3, .timer = 0},
    },

    // Bat: Very Low HP, Low Atk, No Def. Nuisance.
    {
        .name = "Bat",
        .glyph = 'b',
        .color = COLOR_PAIR_ENEMY_BAT,
        .base_hp = 8,
        .base_attack = 2,
        .base_defense = 0,
        .scale_factor = 0.4f,
        .threat_cost = 1,
        .min_depth = 1,
        .base_weight = 40,
        .depth_weight_mod = 0,
        .ai = (AI){.aware = false, .detection_radius = 5},
        .turn_delay = (TurnDelay){.delay = 1, .timer = 0},
    },

    // Snake: Low HP, High Atk, No Def. Starts floor 3.
    {
        .name = "Snake",
        .glyph = 's',
        .color = COLOR_PAIR_ENEMY_SNAKE,
        .base_hp = 15,
        .base_attack = 10,
        .base_defense = 0,
        .scale_factor = 1.2f,
        .threat_cost = 4,
        .min_depth = 3,
        .base_weight = 20,
        .depth_weight_mod = 2,
        .ai = (AI){.aware = false, .detection_radius = 4},
        .turn_delay = (TurnDelay){.delay = 2, .timer = 0},
    },

    // Orc: High HP, High Atk, Med Def. Starts floor 4.
    {
        .name = "Orc",
        .glyph = 'O',
        .color = COLOR_PAIR_ENEMY_ORC,
        .base_hp = 35,
        .base_attack = 8,
        .base_defense = 3,
        .scale_factor = 1.5f,
        .threat_cost = 6,
        .min_depth = 4,
        .base_weight = 10,
        .depth_weight_mod = 5,
        .ai = (AI){.aware = false, .detection_radius = 5},
        .turn_delay = (TurnDelay){.delay = 3, .timer = 0},
    },

    // Zombie: High HP, Low Atk, Low Def. Starts floor 5.
    {
        .name = "Zombie",
        .glyph = 'Z',
        .color = COLOR_PAIR_ENEMY_ZOMBIE,
        .base_hp = 40,
        .base_attack = 4,
        .base_defense = 1,
        .scale_factor = 1.3f,
        .threat_cost = 5,
        .min_depth = 5,
        .base_weight = 5,
        .depth_weight_mod = 5,
        .ai = (AI){.aware = false, .detection_radius = 8},
        .turn_delay = (TurnDelay){.delay = 4, .timer = 0},
    },
};

#endif // ENEMY_H
