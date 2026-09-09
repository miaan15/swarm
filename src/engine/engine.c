#include <stdlib.h>
#include <math.h>

#include "box.h"
#include "context.h"
#include "entity.h"
#include "draw.h"
#include "raylib.h"
#include "sprite.h"

#define SPRITE_COUNT 0

#define BOX_MIN_X 0 + 10
#define BOX_MAX_X 1280 - 10
#define BOX_MIN_Y 0 + 10
#define BOX_MAX_Y 720 - 10

arena omni_arena = {0};

typedef struct {
    f32 vx;
    f32 vy;
} sprite_velocity;

static sprite_velocity spr_vel[SPRITE_COUNT + 1];

static inline f32 rand_f32(f32 min, f32 max) {
    return min + ((f32)rand() / (f32)RAND_MAX) * (max - min);
}

u32 mouse_box_idx;
void engine_init() {
    arena_init(&omni_arena, 100 << 10 << 10); // 100MB

    draw_sys_init(99999, 99999);
    sprite_sys_init(99999, 99999);
    box_sys_init(99999, 50);
    entity_sys_init(2048);

    texture_load("img/char_00.png");
    sprite_prf_make(1, 0,  0, 20, 20); // 1
    sprite_prf_make(1, 0, 20, 20, 20); // 2
    sprite_prf_make(1, 0, 40, 20, 20); // 3
    sprite_prf_make(1, 0, 60, 20, 20); // 4

    for (u32 i = 1; i <= SPRITE_COUNT; ++i) {
        sprite *spr;
        sprite_create(i, &spr);

        spr->prf_idx = (rand() % 4) + 1;
        sprite_prf prf = sprite_prf_get(spr->prf_idx);

        spr->x = rand_f32(BOX_MIN_X, BOX_MAX_X - prf.w);
        spr->y = rand_f32(BOX_MIN_Y, BOX_MAX_Y - prf.h);

        f32 angle = rand_f32(0.0f, 6.2831853f);
        f32 speed = rand_f32(0.1f, 0.8f);
        spr_vel[i].vx = cosf(angle) * speed;
        spr_vel[i].vy = sinf(angle) * speed;
    }

    // TODO debug
    constexpr usize CNT = 5;
    f32 aabb[CNT][4] = { 
        {100, 100, 100, 300},
        {200, 200, 300, 100},
        {520, 270, 250, 200},
        {600, 230, 200, 80},
        {900, 300, 50, 50},
    };
    for (usize i = 0; i < CNT; ++i) {
        box *bx;
        u32 idx = box_create(&bx);
        memcpy(&bx->x, &aabb[i], 4 * sizeof(f32));
        box_add_to_tree(idx);
    }

    {
        box *bx;
        mouse_box_idx = box_create(&bx);
        bx->w = 100;
        bx->h = 100;
        box_add_to_tree(mouse_box_idx);
    }
}

void engine_update() {
    for (u32 i = 1; i <= SPRITE_COUNT; ++i) {
        sprite *spr = sprite_get(i);
        sprite_prf prf = sprite_prf_get(spr->prf_idx);

        spr->x += spr_vel[i].vx;
        spr->y += spr_vel[i].vy;

        if (spr->x <= BOX_MIN_X) {
            spr->x = BOX_MIN_X;
            spr_vel[i].vx = -spr_vel[i].vx;
        } else if (spr->x + prf.w >= BOX_MAX_X) {
            spr->x = BOX_MAX_X - prf.w;
            spr_vel[i].vx = -spr_vel[i].vx;
        }

        if (spr->y <= BOX_MIN_Y) {
            spr->y = BOX_MIN_Y;
            spr_vel[i].vy = -spr_vel[i].vy;
        } else if (spr->y + prf.h >= BOX_MAX_Y) {
            spr->y = BOX_MAX_Y - prf.h;
            spr_vel[i].vy = -spr_vel[i].vy;
        }
    }

    {
        box *bx = box_get(mouse_box_idx);
        bx->x = GetMouseX();
        bx->y = GetMouseY();
    }
    box_sys_update();
}

void engine_input() {

}

void engine_draw() {
    sprite_sys_draw();

    draw_present();

    box_sys_draw_debug();
}

void engine_destroy() {
    arena_destroy(&omni_arena);
}
