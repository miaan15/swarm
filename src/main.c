#include "arena.h"
#include "context.h"
#include "entity.h"
#include "draw.h"
#include "game.h"
#include "log.h"
#include <dlfcn.h>
#include <raylib.h>
#include <unistd.h>

// global stuff
// =============================================================================
// config
f32 screen_width = 1280;
f32 screen_height = 720;

u32 tick_per_second = 20;

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

// functions binding
void (*fn_handle_entity)(entity *) = {0};
void (*fn_handle_action)(action *) = {0};
void (*fn_handle_effect)(effect *) = {0};

// =============================================================================
Camera2D camera = {0};

// hot reload
// =============================================================================
constexpr const char libhandle_path[] = _EXE_DIR "/libhandle.so";
constexpr const char reload_cmd[] = _PROJECT_DIR "/do.sh --reload > /dev/null";
void *libhandle_handle = nullptr;

void engine_game_reload() {
    if (system(reload_cmd) == -1) {
        log_err("engine_game_reload: reload cmd \"%s\" failed", reload_cmd);
        return;
    }

    if (libhandle_handle) { dlclose(libhandle_handle); }

    libhandle_handle = dlopen(libhandle_path, RTLD_NOW | RTLD_GLOBAL);
    if (!libhandle_handle) {
        log_err("engine_game_reload: dlopen error: %s\n", dlerror());
        return;
    }

    dlerror(); // clear old err

    // functions binding // FIXME
    fn_handle_entity = dlsym(libhandle_handle, "handle_entity");
    fn_handle_action = dlsym(libhandle_handle, "handle_action");
    fn_handle_effect = dlsym(libhandle_handle, "handle_effect");

    //
    char *err = dlerror();
    if (err != NULL) {
        log_err("engine_game_reload: dlsym error: %s\n", err);
        dlclose(libhandle_handle);
        libhandle_handle = nullptr;
    }
}

// =============================================================================
int main(void)
{
    // raylib
    InitWindow(screen_width, screen_height, "swarm");

    camera.target = (Vector2){ camera_x, camera_y };
    camera.offset = (Vector2){ screen_width / 2, screen_height / 2 };
    camera.rotation = 0;
    camera.zoom = camera_zoom;

    //
    engine_game_reload();

    // INIT
    game_init();

    // main loop
    u32 time_lastframe_ms = 0;
    while (!WindowShouldClose()) {
        if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) 
            && IsKeyPressed(KEY_R)) {
            engine_game_reload();
            log_info("GAME RELOAD...");
        }

        // cal time
        time_ms = (u32)(GetTime() * 1000);
        time_delta_ms = time_ms - time_lastframe_ms;
        time_lastframe_ms = time_ms;

        tick_accumulate_time_ms += time_delta_ms;

        // INPUT
        game_input();

        // tick handle
        tick_delta_ms = 1000 / tick_per_second;
        // tick should not lower than fps
        if (tick_delta_ms < time_delta_ms) { tick_delta_ms = time_delta_ms; }
        constexpr u32 MIN_TICK_PER_SECOND = 5;
        if (tick_delta_ms > 1000 / MIN_TICK_PER_SECOND) { tick_delta_ms = 1000 / MIN_TICK_PER_SECOND; }

        // if on tick
        while (tick_accumulate_time_ms > tick_delta_ms) {
            tick_accumulate_time_ms -= tick_delta_ms;

            // swap tick arena
            tick_arena = &tick_arena_raw[cur_tick_arena_idx];
            cur_tick_arena_idx = 1 - cur_tick_arena_idx;

            // UPDATE
            game_update();

            // systems update
            action_sys_update();
            effect_sys_update();
            entity_sys_update();
            collider_sys_update();

            // UPDATE LATE
            game_update_late();
        }
        tick_frame_alpha = (f32)tick_accumulate_time_ms / (f32)tick_delta_ms;

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
                sprite_sys_draw();
                draw_present();
            EndMode2D();

            DrawRectangle(0, 0, 110, 40, WHITE);
            DrawFPS(10, 10);
        EndDrawing();
    }

    // DESTROY
    game_destroy();

    texture_destroy_all();

    arena_destroy(&omni_arena);

    CloseWindow();

    return 0;
}
