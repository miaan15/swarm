#include "arena.h"
#include "collider.h"
#include "context.h"
#include "define.h"
#include "draw.h"
#include "entity.h"
#include "log.h"
#include <math.h>
#include <dlfcn.h>
#include <raylib.h>
#include <time.h>
#include <unistd.h>

// context stuff
// =============================================================================
// config
f32 screen_width = 1280;
f32 screen_height = 720;

u32 tick_per_second = 10;

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

// hot reload
// =============================================================================
constexpr const char libgame_path[] = _EXE_DIR "/libgame.so";
constexpr const char reload_cmd[] = _PROJECT_DIR "/do.sh -r > /dev/null";
void *libgame_handle = nullptr;
time_t libgame_last_modified;

void (*game_init_fn)(void);
void (*game_update_fn)(void);
void (*game_update_late_fn)(void);
void (*game_input_fn)(void);
void (*game_visual_fn)(void);
void (*game_draw_fn)(void);
void (*game_destroy_fn)(void);

bool is_game_reloading = false;
bool is_game_just_reloaded = false;

void engine_game_reload() {
    if (system(reload_cmd) == -1) {
        log_err("engine_game_reload: reload cmd \"%s\" failed", reload_cmd);
        return;
    }

    if (libgame_handle) { dlclose(libgame_handle); }

    libgame_handle = dlopen(libgame_path, RTLD_NOW | RTLD_LOCAL);
    if (!libgame_handle) {
        log_err("engine_game_reload: dlopen error: %s\n", dlerror());
        return;
    }

    dlerror(); // clear old err
    game_init_fn = dlsym(libgame_handle, "game_init");
    game_update_fn = dlsym(libgame_handle, "game_update");
    game_update_late_fn = dlsym(libgame_handle, "game_update_late");
    game_input_fn = dlsym(libgame_handle, "game_input");
    game_visual_fn = dlsym(libgame_handle, "game_visual");
    game_draw_fn = dlsym(libgame_handle, "game_draw");
    game_destroy_fn = dlsym(libgame_handle, "game_destroy");

    char *err = dlerror();
    if (err != NULL) {
        log_err("engine_game_reload: dlsym error: %s\n", err);
        dlclose(libgame_handle);
        libgame_handle = nullptr;
    }
}

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

    engine_game_reload();

    game_init_fn(); // INIT

    // main loop
    u32 time_lastframe_ms = 0;
    while (!WindowShouldClose()) {
        if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) 
            && IsKeyPressed(KEY_R)) {
            engine_game_reload();
            log_info("GAME RELOAD...");
        }

        time_ms = (u32)(GetTime() * 1000);
        time_delta_ms = time_ms - time_lastframe_ms;
        time_lastframe_ms = time_ms;

        tick_accumulate_time_ms += time_delta_ms;

        game_input_fn(); // INPUT

        tick_delta_ms = 1000 / tick_per_second;
        if (tick_delta_ms < time_delta_ms) { tick_delta_ms = time_delta_ms; }
        constexpr u32 MIN_TICK_PER_SECOND = 5;
        if (tick_delta_ms > 1000 / MIN_TICK_PER_SECOND) { tick_delta_ms = 1000 / MIN_TICK_PER_SECOND; }
        while (tick_accumulate_time_ms > tick_delta_ms) {
            tick_accumulate_time_ms -= tick_delta_ms;

            tick_arena = &tick_arena_raw[cur_tick_arena_idx];
            cur_tick_arena_idx = 1 - cur_tick_arena_idx;

            game_update_fn(); // UPDATE

            entity_sys_update();
            collider_sys_update();

            game_update_late_fn(); // UPDATE LATE
        }
        tick_frame_alpha = (f32)tick_accumulate_time_ms / (f32)tick_delta_ms;

        game_visual_fn(); // UPDATE VISUAL

        BeginDrawing();
            ClearBackground(RAYWHITE);

            DrawRectangle(0, 0, 110, 40, WHITE);
            DrawFPS(10, 10);

            game_draw_fn(); // DRAW

            sprite_sys_draw();
            draw_present();
        EndDrawing();
    }

    game_destroy_fn(); // DESTROY

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
