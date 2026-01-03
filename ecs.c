#ifndef _ECS_C
#define _ECS_C

#include <ctype.h>
#include <ncurses.h>

#define MAX_ENTITIES (128)

typedef int Entity;

typedef struct {
    int x, y;
} Position;

typedef struct {
    char glyph;
    int color_pair;
} Renderable;

typedef struct {
    int count;

    Entity entities[MAX_ENTITIES];

    Position positions[MAX_ENTITIES];
    Renderable renderables[MAX_ENTITIES];

    bool has_position[MAX_ENTITIES];
    bool has_renderable[MAX_ENTITIES];
} World;

Entity world_create_entity(World *w) {
    Entity e = w->count++;
    w->entities[e] = e;
    w->has_position[e] = false;
    w->has_renderable[e] = false;
    return e;
}

void world_add_position(World *w, Entity e, Position pos) {
    w->positions[e] = pos;
    w->has_position[e] = true;
}

void world_add_renderable(World *w, Entity e, Renderable r) {
    w->renderables[e] = r;
    w->has_renderable[e] = true;
}

#endif // _ECS_C
