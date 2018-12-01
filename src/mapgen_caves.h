#pragma once
#ifndef MAPGEN_CAVES_H
#define MAPGEN_CAVES_H

#include "mapgen_functions.h"


void mapgen_natural_cave_entrance( map *m, oter_id terrain_type, mapgendata dat,
                                   const time_point &turn, float density );
void mapgen_natural_cave( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                          float density );

std::vector<std::vector<int>> rise_automaton( int width, int height, int initial_filled,
                           int birth_thresh, int death_thresh, int iterations );
std::vector<std::vector<int>> go_home_youre_drunk( int width, int height, int limit );
void blooming_booming( int width, int height, std::vector<std::vector<int>> &current );

#endif
