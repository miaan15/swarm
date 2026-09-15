#include "arena.h"
#include "context.h"
#include "entity.h"
#include "draw.h"
#include "game.h"
#include "handle/handle.h"
#include "log.h"
#include <dlfcn.h>
#include <raylib.h>
#include <unistd.h>

// global stuff
// =============================================================================
// config
f32 screen_width = 1280;
f32 screen_height = 720;

u32 tick_per_second = 10;

// var
arena omni_arena = {0};

f32 camera_x = 0;
f32 camera_y = 0;
f32 camera_zoom = 1;

arena tick_arena_raw[2] = { {0}, {0} };
size_t cur_tick_arena_idx = {0};
arena *tick_arena = {0};

u32 time_ms = {0};
u32 time_delta_ms = {0};

u32 tick_cnt = {0};
u32 tick_delta_ms = {0};
f32 tick_frame_alpha = {0};
f32 tick_accumulate_time_ms = {0};
u32 tick_at_this_frame = {0};

// functions binding
void (*fn_handle_entity)(entity *) = {0};
void (*fn_handle_action)(action *) = {0};
void (*fn_handle_status)(status *) = {0};

// bench
clock_t clocks[100] = {0};

// =============================================================================
Camera2D camera = {0};

// TODO: maybe hot-reload later
// // hot reload
// // =============================================================================
// constexpr const char libhandle_path[] = _EXE_DIR "/libhandle.so";
// constexpr const char reload_cmd[] = _PROJECT_DIR "/do.sh --reload > /dev/null";
// void *libhandle_handle = nullptr;
//
// void engine_game_reload() {
//     if (system(reload_cmd) == -1) {
//         log_err("engine_game_reload: reload cmd \"%s\" failed", reload_cmd);
//         return;
//     }
//
//     if (libhandle_handle) { dlclose(libhandle_handle); }
//
//     libhandle_handle = dlopen(libhandle_path, RTLD_NOW | RTLD_GLOBAL);
//     if (!libhandle_handle) {
//         log_err("engine_game_reload: dlopen error: %s\n", dlerror());
//         return;
//     }
//
//     dlerror(); // clear old err
//
//     // functions binding // FIXME
//     fn_handle_entity = dlsym(libhandle_handle, "handle_entity");
//     fn_handle_action = dlsym(libhandle_handle, "handle_action");
//     fn_handle_status = dlsym(libhandle_handle, "handle_status");
//
//     //
//     char *err = dlerror();
//     if (err != NULL) {
//         log_err("engine_game_reload: dlsym error: %s\n", err);
//         dlclose(libhandle_handle);
//         libhandle_handle = nullptr;
//     }
// }

void handle_debug();

// =============================================================================
int main(void)
{
    // arena alloc
    arena_init(&omni_arena, 500 << 10 << 10); // 500MB
    arena_init_in_arena(&tick_arena_raw[0], &omni_arena, 50 << 10 << 10); // 50MB
    arena_init_in_arena(&tick_arena_raw[1], &omni_arena, 50 << 10 << 10); // 50MB
    tick_arena = &tick_arena_raw[cur_tick_arena_idx];

    // raylib
    InitWindow(screen_width, screen_height, "swarm");
    // SetTargetFPS(60);

    camera.target = (Vector2){ camera_x, camera_y };
    camera.offset = (Vector2){ screen_width / 2, screen_height / 2 };
    camera.rotation = 0;
    camera.zoom = camera_zoom;

    //
    // engine_game_reload();
    fn_handle_entity = &handle_entity;
    fn_handle_action = &handle_action;
    fn_handle_status = &handle_status;

    // INIT
    game_init();

    // main loop
    u32 time_lastframe_ms = 0;
    while (!WindowShouldClose()) {
        // if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
        //     && IsKeyPressed(KEY_R)) {
        //     engine_game_reload();
        //     log_info("GAME RELOAD...");
        // }

        // cal time
        time_ms = (u32)(GetTime() * 1000);
        time_delta_ms = time_ms - time_lastframe_ms;
        time_lastframe_ms = time_ms;

        tick_accumulate_time_ms += time_delta_ms;

        // INPUT
        game_input();

        SET_CLOCK(CLOCK_START_TICK);

        // tick handle
        tick_delta_ms = 1000 / tick_per_second;
        // tick should not lower than fps
        if (tick_delta_ms < time_delta_ms) { tick_delta_ms = time_delta_ms; }
        constexpr u32 MIN_TICK_PER_SECOND = 5;
        if (tick_delta_ms > 1000 / MIN_TICK_PER_SECOND) { tick_delta_ms = 1000 / MIN_TICK_PER_SECOND; }

        // if on tick
        while (tick_accumulate_time_ms > tick_delta_ms) {
            tick_accumulate_time_ms -= tick_delta_ms;
            ++tick_at_this_frame;

            // swap tick arena
            cur_tick_arena_idx = 1 - cur_tick_arena_idx;
            tick_arena = &tick_arena_raw[cur_tick_arena_idx];
            arena_reset(tick_arena);

            SET_CLOCK(CLOCK_START_GAME_UPDATE);
            // UPDATE
            game_update();
            SET_CLOCK(CLOCK_END_GAME_UPDATE);

            // systems update
            action_sys_update();
            status_sys_update();

            SET_CLOCK(CLOCK_START_ENTITY_SYS);
            entity_sys_update();
            SET_CLOCK(CLOCK_END_ENTITY_SYS);

            SET_CLOCK(CLOCK_START_COLLIDER_SYS);
            collider_sys_update();
            SET_CLOCK(CLOCK_END_COLLIDER_SYS);

            // UPDATE LATE
            game_update_late();
        }
        tick_frame_alpha = (f32)tick_accumulate_time_ms / (f32)tick_delta_ms;

        SET_CLOCK(CLOCK_END_TICK);

        //
        camera.target = (Vector2){ camera_x, camera_y };
        camera.zoom = camera_zoom;

        // VISUAL
        game_visual();

        BeginDrawing();
            ClearBackground(RAYWHITE);

            BeginMode2D(camera);
                // DRAW
                game_draw();

                // systems draw
                SET_CLOCK(CLOCK_START_PORTRAIT_SYS);
                portrait_sys_update();
                SET_CLOCK(CLOCK_END_PORTRAIT_SYS);

                SET_CLOCK(CLOCK_START_DRAW);
                draw_present();
                SET_CLOCK(CLOCK_END_DRAW);

                handle_debug();
            EndMode2D();

            DrawRectangle(0, 0, 110, 40, WHITE);
            DrawFPS(10, 10);
        EndDrawing();

        // Clean up
        tick_at_this_frame = 0;

#ifdef BENCHMARK
        {
            printf("\n===========================================================\n");
            const char *const CLOCK_LABELS[] = {
                [CLOCK_START_TICK]                = "Tick",
                [CLOCK_START_GAME_UPDATE]         = "Game Update",
                [CLOCK_START_ENTITY_SYS]          = "Entity Sys",
                [CLOCK_START_ENTITY_POS_UPDATE]   = "Entity Pos Update",
                [CLOCK_START_ENTITY_COMPS_UPDATE] = "Entity Comps Update",
                [CLOCK_START_COLLIDER_SYS]        = "Collider Sys",
                [CLOCK_START_PORTRAIT_SYS]        = "Portrait Sys",
                [CLOCK_START_DRAW]                = "Draw",
                [CLOCK_START_DRAW_SORT]           = "Draw Sort",
                [CLOCK_START_DRAW_CALL]           = "Draw Call raylib",
            };
            for (int i = 0; i < 20; i += 2) {
                clock_t start = clocks[i];
                clock_t end   = clocks[i + 1];

                double elapsed_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;

                printf("%-20s: %.4f ms\n", CLOCK_LABELS[i], elapsed_ms);

                if (i == 0 && elapsed_ms < 1) {
                    i += 5 * 2;
                    printf("\n\n\n\n\n");
                }
            }
        }
#endif
    }

    // DESTROY
    game_destroy();

    texture_destroy_all();

    arena_destroy(&omni_arena);

    CloseWindow();

    return 0;
}

typedef struct {
    u32 entity;
    f32 x, y, w, h;
} debug_entity_bounds;

u32 cur_hover_ett = 0;

void handle_debug() {
    debug_entity_bounds hover_list[100];
    usize hover_len = 0;

    Vector2 screen_mouse = GetMousePosition();
    Vector2 mouse_pos = {
        .x = (screen_mouse.x - camera.offset.x) / camera.zoom + camera.target.x,
        .y = (screen_mouse.y - camera.offset.y) / camera.zoom + camera.target.y,
    };

    for (usize i = 0; i < (entity_sys.entity_max_idx + 7) / 8; ++i) {
        entity_pos_soa *soa = &entity_sys.pos_soa_pool[i];
        for (usize j = 0; j < 8; ++j) {
            f32 dx = -(soa->x[j] - soa->last_x[j]) * (1 - tick_frame_alpha);
            f32 dy = -(soa->y[j] - soa->last_y[j]) * (1 - tick_frame_alpha);

            u32 idx = i * 8 + j;
            if (idx > 0 && idx < entity_sys.entity_max_idx) {
                f32 x, y, w, h;
                entity_cal_bounds(idx, &x, &y, &w, &h);

                f32 final_x = x + dx;
                f32 final_y = y + dy;

                Rectangle rect = (Rectangle){final_x, final_y, w, h};
                DrawRectangleLinesEx(rect, .5f, (Color){255, 0, 0, 50});

                if (mouse_pos.x >= final_x &&
                    mouse_pos.x <= final_x + w &&
                    mouse_pos.y >= final_y &&
                    mouse_pos.y <= final_y + h
                    && hover_len < 100) {
                    hover_list[hover_len++] = (debug_entity_bounds){
                        .entity = idx,
                        .x = final_x,
                        .y = final_y,
                        .w = w,
                        .h = h,
                    };
                }
            }
        }
    }

    // if (cur_hover_ett != 0) {
    //     for (usize i = 0; i < hover_len; ++i) {
    //         if (hover_list[i].entity == cur_hover_ett) break;
    //         if (i == hover_len - 1) cur_hover_ett = 0;
    //     }
    // }
    cur_hover_ett = 0;

    if (cur_hover_ett == 0 && hover_len > 0) {
        u64 best_meta = 0;
        u32 best_idx = 0;

        for (usize i = 0; i < hover_len; ++i) {
            u32 ett_id = hover_list[i].entity;
            entity *ett = entity_get(ett_id);

            u64 meta = 0;

            u8 ett_uz = (u8)ett->z ^ 0x80;
            meta |= (u64)ett_uz << 56;

            f32 ett_y;
            entity_pos_get(ett_id, 0, &ett_y, 0, 0, 0, 0);
            u32 ett_uy; memcpy(&ett_uy, &ett_y, sizeof(f32));
            ett_uy ^= (-(i32)(ett_uy >> 31) | 0x80000000u);
            meta |= (u64)ett_uy << 24;

            if (best_idx == 0 || meta > best_meta) {
                best_meta = meta;
                best_idx = ett_id;
            }
        }
        cur_hover_ett = best_idx;
    }

    if (cur_hover_ett != 0) {
        for (usize i = 0; i < hover_len; ++i) {
            if (hover_list[i].entity == cur_hover_ett) {
                DrawRectangleLinesEx(
                    (Rectangle){hover_list[i].x, hover_list[i].y, hover_list[i].w, hover_list[i].h},
                    2.0f,
                    (Color){0, 255, 128, 255}
                );
                break;
            }
        }
    }
}
