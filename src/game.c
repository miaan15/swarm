#include "game.h"

#include "context.h"
#include "draw.h"
#include "entity.h"
#include <raylib.h>

void game_init() {
    // arena alloc
    arena_init(&omni_arena, 500 << 10 << 10); // 500MB
    arena_init_in_arena(&tick_arena_raw[0], &omni_arena, 50 << 10 << 10); // 50MB
    arena_init_in_arena(&tick_arena_raw[1], &omni_arena, 50 << 10 << 10); // 50MB
    tick_arena = &tick_arena_raw[cur_tick_arena_idx];

    // engine set up
    draw_sys_init(1e2, 1e5);

    sprite_sys_init(1e5, 1e5);
    collider_sys_init(1e5, 30.0f);
    effect_sys_init(1e5);
    entity_sys_init(1e5);

    action_sys_init(1e5);
}

void game_input() { }

void game_update() { }

void game_update_late() { }

void game_visual() { }

void game_draw() { }

void game_destroy() { }
