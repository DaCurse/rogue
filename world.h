#ifndef WORLD_H
#define WORLD_H

#include <ncurses.h>
#include <stdbool.h>

#include "config.h"
#include "floor.h"

#define MAX_ENTITIES (128)
#define LOG_MESSAGE_SIZE (MAP_W)
#define INVALID_ENTITY (-1)

typedef int Entity;

typedef struct {
    WINDOW *win;
} RenderContext;

typedef struct {
    int x, y;
} Position;

typedef struct {
    char glyph;
    int color_pair;
} Renderable;

typedef struct {
} Player;

typedef struct {
    char name[16];
    int hp;
    int max_hp;
    int attack;
    int defense;
} CombatStats;

typedef struct {
    bool blocks_movement;
} Collider;

typedef enum { COLLISION_NONE, COLLISION_MAP, COLLISION_ENTITY } CollisionType;

typedef struct {
    CollisionType type;
    Entity entity; // valid if type == COLLISION_ENTITY
} CollisionResult;

typedef struct {
    int dx, dy;
} MoveIntent;

typedef struct {
    Entity attacker, defender;
} CombatIntent;

typedef struct {
    unsigned int seed;
    Floor *floor;
    RenderContext map;
    RenderContext log_window;
    RenderContext status_bar;

    int count;
    Entity entities[MAX_ENTITIES];

    Entity player;
    Player player_data;

    char log_message[LOG_MESSAGE_SIZE];
    int log_repeat_count;

    Position positions[MAX_ENTITIES];
    Renderable renderables[MAX_ENTITIES];
    CombatStats combat_stats[MAX_ENTITIES];
    Collider colliders[MAX_ENTITIES];
    MoveIntent move_intents[MAX_ENTITIES];
    CombatIntent combat_intents[MAX_ENTITIES];

    bool has_position[MAX_ENTITIES];
    bool has_renderable[MAX_ENTITIES];
    bool has_combat_stats[MAX_ENTITIES];
    bool has_collider[MAX_ENTITIES];
    bool has_move_intent[MAX_ENTITIES];
    bool has_combat_intent[MAX_ENTITIES];
} World;

Entity world_create_entity(World *w);
void world_add_position(World *w, Entity e, Position pos);
void world_add_renderable(World *w, Entity e, Renderable r);
void world_add_combat_stats(World *w, Entity e, CombatStats cs);
void world_add_collider(World *w, Entity e, Collider c);
void world_add_move_intent(World *w, Entity e, MoveIntent mi);
void world_add_combat_intent(World *w, Entity e, CombatIntent ci);
void world_logf(World *w, const char *format, ...);
Renderable tile_glyph(Floor *f, int x, int y);

#endif // WORLD_H
