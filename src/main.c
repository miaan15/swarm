#include "action.h"
#include "arena.h"
#include "collider.h"
#include "context.h"
#include "define.h"
#include "draw.h"
#include "entity.h"
#include "log.h"
#include <dlfcn.h>
#include <raylib.h>
#include <time.h>
#include <unistd.h>

// extern stuff
// =============================================================================
// proxy
void (*action_handle_fn)(action) = nullptr;
void (*effect_handle_fn)(effect *) = nullptr;

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
    game_init_fn        = dlsym(libgame_handle, "game_init");
    game_update_fn      = dlsym(libgame_handle, "game_update");
    game_update_late_fn = dlsym(libgame_handle, "game_update_late");
    game_input_fn       = dlsym(libgame_handle, "game_input");
    game_visual_fn      = dlsym(libgame_handle, "game_visual");
    game_draw_fn        = dlsym(libgame_handle, "game_draw");
    game_destroy_fn     = dlsym(libgame_handle, "game_destroy");

    action_handle_fn    = dlsym(libgame_handle, "game_action_handle");
    effect_handle_fn    = dlsym(libgame_handle, "game_effect_handle");

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
    effect_sys_init(1e5);
    entity_sys_init(1e5);
    
    action_sys_init(1e5);

    //
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

            action_sys_update();
            effect_sys_update();
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
