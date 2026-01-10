#ifndef GAME_H
#define GAME_H

#include "world.h"

void create_player(World *world);
void create_enemy(World *world);
void add_room_exit(World *world);
void setup_new_level(World *world);

#endif
