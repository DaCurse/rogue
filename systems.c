#ifndef SYSTEMS_C
#define SYSTEMS_C

#include <ncurses.h>

#include "ecs.c"
#include "floor.c"

void system_render(WINDOW *win, World *w) {
    for (int e = 0; e < w->count; e++) {
        if (!(w->has_position[e] && w->has_renderable[e]))
            continue;

        Position *p = &w->positions[e];
        Renderable *r = &w->renderables[e];

        mvwaddch(win, p->y, p->x, r->glyph | COLOR_PAIR(r->color_pair));
    }
}

void system_player_input(World *w, Entity player, char key, Floor *f) {
    Position *p = &w->positions[player];

    int nx = p->x;
    int ny = p->y;

    switch (tolower(key)) {
    case 'w':
        ny--;
        break;
    case 's':
        ny++;
        break;
    case 'a':
        nx--;
        break;
    case 'd':
        nx++;
        break;
    default:
        return;
    }

    Tile t = tile_at(f, nx, ny);
    if (t == TILE_FLOOR || t == TILE_ROAD) {
        p->x = nx;
        p->y = ny;
        floor_reveal_area(f, nx, ny, 5);
    }
}

#endif // SYSTEMS_C