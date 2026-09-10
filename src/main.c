#include "arena.h"
#include "collider.h"
#include "context.h"
#include "draw.h"
#include "entity.h"
#include "log.h"
#include <math.h>
#include <raylib.h>

// context stuff
// =============================================================================
// config
f32 screen_width = 1280;
f32 screen_height = 720;

u32 tick_per_second = 50;

// var
arena omni_arena = {0};

arena tick_arena_raw[2] = { {0}, {0} };
size_t cur_tick_arena_idx = {0};
arena *tick_arena = {0};

u32 time_ms = {0};
u32 time_delta_ms = {0};

u32 tick_cnt = {0};
u32 tick_delta_ms = {0};
f32 tick_frame_alpha = {0};
f32 tick_accumulate_time_ms = {0};

// =============================================================================
int main(void)
{
    // arena alloc
    arena_init(&omni_arena, 100 << 10 << 10); // 100MB
    arena_init_in_arena(&tick_arena_raw[0], &omni_arena, 10 << 10 << 10); // 10MB
    arena_init_in_arena(&tick_arena_raw[1], &omni_arena, 10 << 10 << 10); // 10MB
    tick_arena = &tick_arena_raw[cur_tick_arena_idx];

    // raylib window
    InitWindow(screen_width, screen_height, "swarm");

    // engine set up
    draw_sys_init(1e2, 1e5);
    sprite_sys_init(1e5, 1e5);
    collider_sys_init(1e5, 30.0f);
    entity_sys_init(1e5);

    // main loop
    u32 time_lastframe_ms = 0;
    while (!WindowShouldClose()) {
        time_ms = (u32)(GetTime() * 1000);
        time_delta_ms = time_ms - time_lastframe_ms;
        time_lastframe_ms = time_ms;

        tick_accumulate_time_ms += time_delta_ms;

        // input here

        tick_delta_ms = 1000 / tick_per_second;
        if (tick_delta_ms < time_delta_ms) { tick_delta_ms = time_delta_ms; }
        constexpr u32 MIN_TICK_PER_SECOND = 5;
        if (tick_delta_ms > 1000 / MIN_TICK_PER_SECOND) { tick_delta_ms = 1000 / MIN_TICK_PER_SECOND; }
        while (tick_accumulate_time_ms > tick_delta_ms) {
            tick_accumulate_time_ms -= tick_delta_ms;

            tick_arena = &tick_arena_raw[cur_tick_arena_idx];
            cur_tick_arena_idx = 1 - cur_tick_arena_idx;

            // update here

            entity_sys_update();
            collider_sys_update();

            // clear input here
        }
        tick_frame_alpha = (f32)tick_accumulate_time_ms / (f32)tick_delta_ms;

        // visual update here

        BeginDrawing();
            ClearBackground(RAYWHITE);

            DrawRectangle(0, 0, 110, 40, WHITE);
            DrawFPS(10, 10);

            // draw here

            sprite_sys_draw();
            draw_present();
        EndDrawing();
    }

    // destroy here

    texture_destroy_all();

    arena_destroy(&omni_arena);

    CloseWindow();

    return 0;
}
//
// #define ENTITY_COUNT 1000
//
// #define MIN_X 0 + 10
// #define MAX_X 1280 - 10
// #define MIN_Y 0 + 10
// #define MAX_Y 720 - 10
//
// typedef struct {
//     f32 vx;
//     f32 vy;
// } ett_velocity;
//
// static ett_velocity ett_vel[ENTITY_COUNT + 1];
//
// static inline f32 rand_f32(f32 min, f32 max) {
//     return min + ((f32)rand() / (f32)RAND_MAX) * (max - min);
// }
//
// void engine_init() {
//
//
//     texture_load("img/char_00.png");
//     sprite_profile_make(1, 0,  0, 20, 20); // 1
//     sprite_profile_make(1, 0, 20, 20, 20); // 2
//     sprite_profile_make(1, 0, 40, 20, 20); // 3
//     sprite_profile_make(1, 0, 60, 20, 20); // 4
//
//     for (u32 i = 1; i <= ENTITY_COUNT; ++i) {
//         entity *ett;
//         u32 ett_idx = entity_create(&ett);
//
//         sprite *spr;
//         entity_add_sprite(ett_idx, (rand() % 4) + 1, &spr);
//
//         collider *col;
//         entity_add_collider(ett_idx, &col);
//
//         sprite_profile spr_prf = sprite_profile_get(spr->profile_idx);
//         f32 ext_x = spr_prf.w / 2, ext_y = spr_prf.h / 2;
//
//         ett->x = rand_f32(MIN_X, MAX_X - ext_x);
//         ett->y = rand_f32(MIN_Y, MAX_Y - ext_y);
//
//         spr->offset_x = -ext_x;
//         spr->offset_y = -ext_y;
//
//         col->offset_x = -ext_x;
//         col->offset_y = -ext_y;
//         col->w = ext_x * 2;
//         col->h = ext_y * 2;
//
//         //
//         f32 angle = rand_f32(0.0f, 6.2831853f);
//         f32 speed = rand_f32(0.1f, 0.8f);
//         ett_vel[i].vx = cosf(angle) * speed;
//         ett_vel[i].vy = sinf(angle) * speed;
//     }
// }
//
// void engine_update() {
//     for (u32 i = 1; i <= ENTITY_COUNT; ++i) {
//         entity *ett = entity_get(i);
//
//         ett->x += ett_vel[i].vx;
//         ett->y += ett_vel[i].vy;
//
//         if (ett->x <= MIN_X) {
//             ett->x = MIN_X;
//             ett_vel[i].vx = -ett_vel[i].vx;
//         } else if (ett->x >= MAX_X) {
//             ett->x = MAX_X;
//             ett_vel[i].vx = -ett_vel[i].vx;
//         }
//
//         if (ett->y <= MIN_Y) {
//             ett->y = MIN_Y;
//             ett_vel[i].vy = -ett_vel[i].vy;
//         } else if (ett->y >= MAX_Y) {
//             ett->y = MAX_Y;
//             ett_vel[i].vy = -ett_vel[i].vy;
//         }
//     }
//
// }
//
// void engine_input() {
//
// }
//
// void engine_draw() {
//
//     // collider_sys_draw_debug();
// }
//
// void engine_destroy() {
//
// }
