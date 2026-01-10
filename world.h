#ifndef WORLD_H
#define WORLD_H

#include <ncurses.h>
#include <stdbool.h>

#include "color.h"
#include "config.h"
#include "floor.h"

#define MAX_ENTITIES (128)
#define LOG_MESSAGE_SIZE (MAP_W)
#define INVALID_ENTITY (-1)
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
    MoveIntent move_intents[MAX_ENTITIES];
    CollisionEvent collision_events[MAX_ENTITIES];

    bool has_position[MAX_ENTITIES];
    bool has_renderable[MAX_ENTITIES];
    bool has_combat_stats[MAX_ENTITIES];
    bool has_collider[MAX_ENTITIES];
    bool has_move_intent[MAX_ENTITIES];
    bool has_collision_event[MAX_ENTITIES];
} World;

Entity world_create_entity(World *w);
void world_add_position(World *w, Entity e, Position pos);
void world_add_renderable(World *w, Entity e, Renderable r);
void world_add_combat_stats(World *w, Entity e, CombatStats cs);
void world_add_collider(World *w, Entity e, Collider c);
void world_add_move_intent(World *w, Entity e, MoveIntent mi);
void world_add_collision_event(World *w, Entity e, CollisionEvent ce);
void world_remove_entity(World *w, Entity e);
bool world_is_occupied(World *w, int x, int y);
int world_get_unoccupied_positions(World *w, Room *r, Position *out_arr,
                                   int max_len);
bool world_get_random_unoccupied_in_room(World *w, Room *r, Position *out_pos);
void world_logf(World *w, const char *format, ...);

#endif // WORLD_H
