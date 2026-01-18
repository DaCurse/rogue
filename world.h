#ifndef WORLD_H
#define WORLD_H

#include <assert.h>
#include <ncurses.h>
#include <stdbool.h>

#include "color.h"
#include "config.h"
#include "floor.h"

#define MAX_ENTITIES (128)
#define INVALID_ENTITY ((Entity)(-1))
#define LOG_MESSAGE_SIZE (MAP_W)
#define PLAYER_NAME_MAX_LENGTH (16)

typedef int Entity;

typedef struct {
    WINDOW *win;
} RenderContext;

typedef struct {
    int x, y;
} Position;

typedef struct {
    char glyph;
    ColorPair color_pair;
} Renderable;

typedef struct {
    int input;
    int room_id;
    int floor;
    bool game_over;
    int turn_count;
} PlayerData;

typedef struct {
    char name[PLAYER_NAME_MAX_LENGTH];
    int hp;
    int max_hp;
    int attack;
    int defense;
} CombatStats;

typedef struct {
    bool blocks_movement;
} Collider;

typedef struct {
    bool aware;
    int detection_radius;
} AI;

typedef struct {
    int delay;
    int timer;
} TurnDelay;

typedef enum { COLLISION_NONE, COLLISION_MAP, COLLISION_ENTITY } CollisionType;

typedef struct {
    CollisionType type;
    Entity entity; // valid if type == COLLISION_ENTITY
} CollisionResult;

typedef struct {
    int dx, dy;
} MoveIntent;

typedef struct {
    Entity target;
} CollisionEvent;

typedef struct {
    char messages[LOG_H][LOG_MESSAGE_SIZE];
    int repeat_counts[LOG_H];
    int turn_timestamp;
} MessageLog;

typedef struct {
    bool position        : 1;
    bool renderable      : 1;
    bool combat_stats    : 1;
    bool collider        : 1;
    bool ai              : 1;
    bool turn_delay      : 1;
    bool move_intent     : 1;
    bool collision_event : 1;
} ComponentFlags;

typedef struct {
    unsigned int seed;
    RenderContext map;
    RenderContext log_window;
    RenderContext status_bar;

    int count;
    Entity entities[MAX_ENTITIES];

    Floor *floor;
    Entity room_exit;

    Entity player;
    PlayerData player_data;

    MessageLog log;

    Position positions[MAX_ENTITIES];
    Renderable renderables[MAX_ENTITIES];
    CombatStats combat_stats[MAX_ENTITIES];
    Collider colliders[MAX_ENTITIES];
    AI ais[MAX_ENTITIES];
    TurnDelay turn_delays[MAX_ENTITIES];
    MoveIntent move_intents[MAX_ENTITIES];
    CollisionEvent collision_events[MAX_ENTITIES];

    ComponentFlags has[MAX_ENTITIES];

    // Hot lists to optimize system iterations
    Entity movers[MAX_ENTITIES];
    int movers_count;

    Entity render_list[MAX_ENTITIES];
    int render_list_count;

    Entity combatants[MAX_ENTITIES];
    int combatants_count;

    Entity collisions[MAX_ENTITIES];
    int collisions_count;

    Entity ai_list[MAX_ENTITIES];
    int ai_list_count;

    Entity turn_delay_list[MAX_ENTITIES];
    int turn_delay_list_count;

    // Spatial index for quick occupancy checks
    Entity entity_at[MAP_H][MAP_W];
} World;

Entity world_create_entity(World *w);
void world_move_entity(World *w, Entity e, int new_x, int new_y);
void world_add_position(World *w, Entity e, Position pos);
void world_add_renderable(World *w, Entity e, Renderable r);
void world_add_combat_stats(World *w, Entity e, CombatStats cs);
void world_add_collider(World *w, Entity e, Collider c);
void world_add_ai(World *w, Entity e, AI ai);
void world_add_turn_delay(World *w, Entity e, TurnDelay td);
void world_add_move_intent(World *w, Entity e, MoveIntent mi);
void world_add_collision_event(World *w, Entity e, CollisionEvent ce);
void world_remove_entity(World *w, Entity e);
int world_get_unoccupied_positions(World *w, Room *r, Position *out_arr,
                                   int max_len);
bool world_get_random_unoccupied_in_room(World *w, Room *r, Position *out_pos);
void world_logf(World *w, const char *format, ...);

// Debug assertion macros for iteration macros below
#define CHECK_COMPONENT_render_list(w, e)                                      \
    assert((w)->has[e].position && (w)->has[e].renderable)
#define CHECK_COMPONENT_movers(w, e)                                           \
    assert((w)->has[e].position && (w)->has[e].move_intent)
#define CHECK_COMPONENT_combatants(w, e) assert((w)->has[e].combat_stats)
#define CHECK_COMPONENT_collisions(w, e) assert((w)->has[e].collision_event)
#define CHECK_COMPONENT_ai_list(w, e) assert((w)->has[e].ai)
#define CHECK_COMPONENT_turn_delay_list(w, e) assert((w)->has[e].turn_delay)

// Iterates over all active entities in a hot list
// Requires: world->LIST[MAX_ENTITIES], world->LIST_count
// Note: 'var' is declared inside the macro
#define FOR_EACH_ACTIVE(world, list, var)                                      \
    for (int _i_##list = 0, var; _i_##list < (world)->list##_count &&          \
                                 ((var) = (world)->list[_i_##list],            \
                                 CHECK_COMPONENT_##list(world, var), 1);       \
         ++_i_##list)

// Iterates over all active entities in a hot list in reverse order
// Requires: world->LIST[MAX_ENTITIES], world->LIST_count
// Note: 'var' is declared inside the macro
#define FOR_EACH_ACTIVE_REVERSE(world, list, var)                              \
    for (int _i_##list = (world)->list##_count - 1, var;                       \
         _i_##list >= 0 && ((var) = (world)->list[_i_##list],                  \
                           CHECK_COMPONENT_##list(world, var), 1);             \
         --_i_##list)

// Iterates over all valid entities (0 to count-1).
#define FOR_EACH_ENTITY(world, var)                                            \
    for (int var = 0; var < (world)->count; ++var)

// Iterates over all entities with a single component predicate.
#define FOR_EACH_ENTITY_IF1(world, var, comp1)                                 \
    for (int var = 0; var < (world)->count; ++var)                             \
        if ((world)->has[var].comp1)

// Iterates over all entities with two component predicates.
#define FOR_EACH_ENTITY_IF2(world, var, comp1, comp2)                          \
    for (int var = 0; var < (world)->count; ++var)                             \
        if ((world)->has[var].comp1 && (world)->has[var].comp2)

// Iterates over all entities with three component predicates.
#define FOR_EACH_ENTITY_IF3(world, var, comp1, comp2, comp3)                   \
    for (int var = 0; var < (world)->count; ++var)                             \
        if ((world)->has[var].comp1 && (world)->has[var].comp2 &&              \
            (world)->has[var].comp3)

#endif // WORLD_H
