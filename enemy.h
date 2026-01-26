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

extern const EnemyTemplate enemy_templates[];

#endif // ENEMY_H
