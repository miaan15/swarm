module;

#include <cstdio>
#include <SDL3/SDL_render.h>

export module context;

import def;
import mem;

export namespace sw {

// CONFIG
constexpr u32 TPS = 20;

constexpr usize OMNI_ARENA_SIZE = 500 << 10 << 10;
constexpr usize TICK_ARENA_SIZE = 50 << 10 << 10;
constexpr usize FRAME_ARENA_SIZE = 50 << 10 << 10;

// DIRECTORIES
char project_dir[512];
char source_dir[512];
char assets_dir[512];

#ifndef _PROJECT_DIR
    #error "_PROJECT_DIR must be defined";
#endif

#if defined(_WIN32) || defined(_WIN64)
    constexpr const char PATH_SEPARATOR[] = "\\";
#else
    constexpr const char PATH_SEPARATOR[] = "/";
#endif

// SDL
SDL_Window *window;
SDL_Renderer *renderer;

// ARENA
arena omni_arena;

arena tick_arena_raw[2];
usize tick_arena_cur_idx;
arena *tick_arena_ptr;

arena frame_arena_raw[2];
usize frame_arena_cur_idx;
arena *frame_arena_ptr;

// TIME
f64 time_sec, time_delta_sec;

f64 tick_delta_sec;
f64 tick_frame_alpha;
f64 tick_accumulated_time_sec;
u32 tick_count_this_frame;

void context_init() {
    // DIRECTORIES
    snprintf(project_dir, sizeof(project_dir), "%s", _PROJECT_DIR);
    snprintf(source_dir, sizeof(source_dir), "%s%s%s", project_dir, PATH_SEPARATOR, "src");
    snprintf(assets_dir, sizeof(assets_dir), "%s%s%s", project_dir, PATH_SEPARATOR, "assets");

    // ARENA
    arena_init(&omni_arena, OMNI_ARENA_SIZE);

    arena_init_in_arena(&tick_arena_raw[0], &omni_arena, TICK_ARENA_SIZE);
    arena_init_in_arena(&tick_arena_raw[1], &omni_arena, TICK_ARENA_SIZE);
    tick_arena_cur_idx = 0;
    tick_arena_ptr = &tick_arena_raw[tick_arena_cur_idx];

    arena_init_in_arena(&frame_arena_raw[0], &omni_arena, FRAME_ARENA_SIZE);
    arena_init_in_arena(&frame_arena_raw[1], &omni_arena, FRAME_ARENA_SIZE);
    frame_arena_cur_idx = 0;
    frame_arena_ptr = &frame_arena_raw[frame_arena_cur_idx];
}

}
