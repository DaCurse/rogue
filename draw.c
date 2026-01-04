#ifndef _DRAW_C
#define _DRAW_C

#include <ncurses.h>

#include "color.c"
#include "ecs.c"
#include "floor.c"
#include "systems.c"

Renderable tile_glyph(Floor *f, int x, int y) {
    switch (tile_at(f, x, y)) {
    case TILE_VOID:
        return (Renderable){.glyph = ' ', .color_pair = COLOR_VOID};
    case TILE_FLOOR:
        return (Renderable){.glyph = '.', .color_pair = COLOR_FLOOR};
    case TILE_ROAD:
        return (Renderable){.glyph = ' ', .color_pair = COLOR_ROAD};
    case TILE_WALL:
        switch (f->walls[x + f->width * y]) {
        case WALL_VERTICAL:
            return (Renderable){.glyph = '|', .color_pair = COLOR_WALL};
        case WALL_HORIZONTAL:
            return (Renderable){.glyph = '_', .color_pair = COLOR_WALL};
        case WALL_CORNER:
            return (Renderable){.glyph = '+', .color_pair = COLOR_WALL};
        default:
            return (Renderable){.glyph = '?', .color_pair = COLOR_UNKNOWN};
        }
    default:
        return (Renderable){.glyph = '?', .color_pair = COLOR_UNKNOWN};
    }
}

void draw_map(WINDOW *win, Floor *f, World *w) {
    werase(win);

    for (int y = 0; y < f->height; y++) {
        for (int x = 0; x < f->width; x++) {
            if (!f->fog_of_war[x + f->width * y])
                continue;

            Renderable r = tile_glyph(f, x, y);
            mvwaddch(win, y, x, r.glyph | COLOR_PAIR(r.color_pair));
        }
    }

    system_render(win, w);

    wrefresh(win);
}

void draw_status_bar(WINDOW *win) {
    werase(win);

    wbkgd(win, COLOR_PAIR(COLOR_STATUS));
    mvwprintw(win, 0, 1, "Level: 1 Floor: 1 Gold: 0");

    wrefresh(win);
}

void draw_logs(WINDOW *win) {
    werase(win);
    box(win, 0, 0);
    mvwprintw(win, 0, 2, " Journal ");
    wrefresh(win);
}

#endif // _DRAW_C