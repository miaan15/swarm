#pragma once

#include "proxy.h"

void game_init();
void game_update();
void game_update_late();
void game_input();
void game_visual();
void game_draw();
void game_destroy();

void game_action_handle(action act);
void game_effect_handle(effect *eff);
