#include <math.h>
#include <stdlib.h>

#include "context.h"
#include "entity.h"
#include "draw.h"

#define ENTITY_COUNT 10000

#define MIN_X 0 + 10
#define MAX_X 1280 - 10
#define MIN_Y 0 + 10
#define MAX_Y 720 - 10

arena omni_arena = {0};

typedef struct {
    f32 vx;
    f32 vy;
} ett_velocity;

static ett_velocity ett_vel[ENTITY_COUNT + 1];

static inline f32 rand_f32(f32 min, f32 max) {
    return min + ((f32)rand() / (f32)RAND_MAX) * (max - min);
}

void engine_init() {
    arena_init(&omni_arena, 100 << 10 << 10); // 100MB

    draw_sys_init(99999, 99999);
    sprite_sys_init(99999, 99999);
    collider_sys_init(99999, 30.0f);
    entity_sys_init(99999);

    texture_load("img/char_00.png");
    sprite_profile_make(1, 0,  0, 20, 20); // 1
    sprite_profile_make(1, 0, 20, 20, 20); // 2
    sprite_profile_make(1, 0, 40, 20, 20); // 3
    sprite_profile_make(1, 0, 60, 20, 20); // 4

    for (u32 i = 1; i <= ENTITY_COUNT; ++i) {
        entity *ett;
        u32 ett_idx = entity_create(&ett);

        sprite *spr;
        entity_add_sprite(ett_idx, (rand() % 4) + 1, &spr);

        collider *col;
        entity_add_collider(ett_idx, &col);

        sprite_profile spr_prf = sprite_profile_get(spr->profile_idx);
        f32 ext_x = spr_prf.w / 2, ext_y = spr_prf.h / 2;

        ett->x = rand_f32(MIN_X, MAX_X - ext_x);
        ett->y = rand_f32(MIN_Y, MAX_Y - ext_y);
        
        spr->offset_x = -ext_x;
        spr->offset_y = -ext_y;

        col->offset_x = -ext_x;
        col->offset_y = -ext_y;
        col->w = ext_x * 2;
        col->h = ext_y * 2;

        //
        f32 angle = rand_f32(0.0f, 6.2831853f);
        f32 speed = rand_f32(0.1f, 0.8f);
        ett_vel[i].vx = cosf(angle) * speed;
        ett_vel[i].vy = sinf(angle) * speed;
    }
}

void engine_update() {
    for (u32 i = 1; i <= ENTITY_COUNT; ++i) {
        entity *ett = entity_get(i);

        ett->x += ett_vel[i].vx;
        ett->y += ett_vel[i].vy;

        if (ett->x <= MIN_X) {
            ett->x = MIN_X;
            ett_vel[i].vx = -ett_vel[i].vx;
        } else if (ett->x >= MAX_X) {
            ett->x = MAX_X;
            ett_vel[i].vx = -ett_vel[i].vx;
        }

        if (ett->y <= MIN_Y) {
            ett->y = MIN_Y;
            ett_vel[i].vy = -ett_vel[i].vy;
        } else if (ett->y >= MAX_Y) {
            ett->y = MAX_Y;
            ett_vel[i].vy = -ett_vel[i].vy;
        }
    }

    entity_sys_update();

    collider_sys_update();
}

void engine_input() {

}

void engine_draw() {
    sprite_sys_draw();

    draw_present();

    // collider_sys_draw_debug();
}

void engine_destroy() {
    arena_destroy(&omni_arena);
}
