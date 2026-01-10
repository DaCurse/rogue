#ifndef SYSTEMS_H
#define SYSTEMS_H

#include "world.h"

void system_render_map(World *w);
void system_render_logs(World *w);
void system_render_status_bar(World *w);
void system_player_input(World *w, char key);
CollisionResult check_collision(World *w, Floor *f, Entity e1, int target_x,
                                int target_y);
void system_movement(World *w);
void system_combat(World *w);
void system_death(World *w);

#endif // SYSTEMS_H
