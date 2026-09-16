#pragma once

#include "arena.h"
#include "define.h"
#include "entity.h"
#include <time.h>

// config stuff
extern f32 screen_width;
extern f32 screen_height;

extern u32 tick_per_second;

// var stuff
extern arena omni_arena;

extern f32 camera_x;
extern f32 camera_y;
extern f32 camera_zoom;

extern arena tick_arena_raw[2];
extern size_t cur_tick_arena_idx;
extern arena *tick_arena;

extern u32 time_ms;
extern u32 time_delta_ms;

extern u32 tick_cnt;
extern u32 tick_delta_ms;
extern f32 tick_frame_alpha;
extern f32 tick_accumulate_time_ms;
extern u32 tick_at_this_frame;

// functions binding
extern void (*fn_handle_entity)(entity *);
extern void (*fn_handle_action)(action *);
extern void (*fn_handle_status)(status *);

// bench
#ifdef BENCHMARK
#include <raylib.h>
extern double clocks[100];
enum {
    CLOCK_START_TICK,
    CLOCK_END_TICK,
    CLOCK_START_GAME_UPDATE,
    CLOCK_END_GAME_UPDATE,
    CLOCK_START_ENTITY_SYS,
    CLOCK_END_ENTITY_SYS,
    CLOCK_START_ENTITY_POS_UPDATE,
    CLOCK_END_ENTITY_POS_UPDATE,
    CLOCK_START_ENTITY_COMPS_UPDATE,
    CLOCK_END_ENTITY_COMPS_UPDATE,
    CLOCK_START_COLLIDER_SYS,
    CLOCK_END_COLLIDER_SYS,
    CLOCK_START_PORTRAIT_SYS,
    CLOCK_END_PORTRAIT_SYS,
    CLOCK_START_DRAW,
    CLOCK_END_DRAW,
    CLOCK_START_DRAW_SORT,
    CLOCK_END_DRAW_SORT,
    CLOCK_START_DRAW_CALL,
    CLOCK_END_DRAW_CALL,
};
#endif

#ifdef BENCHMARK
    #define SET_CLOCK(i) do { \
        clocks[i] = GetTime(); \
    } while (0)
#else
    #define SET_CLOCK(i) do { } while (0)
#endif
