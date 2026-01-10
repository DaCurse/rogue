#ifndef SYSTEMS_H
#define SYSTEMS_H

#include "world.h"

void system_render_map(World *w);
void system_render_logs(World *w);
void system_render_status_bar(World *w);
void system_player_input(World *w, int ch);
void system_movement(World *w);
void system_combat(World *w);
void system_exit_room(World *w);
void system_death(World *w);

#endif // SYSTEMS_H
