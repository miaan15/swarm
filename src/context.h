#pragma once

#include "arena.h"
#include "define.h"
#include "raylib.h"

// config stuff
extern f32 screen_width;
extern f32 screen_height;

extern u32 tick_per_second;

// var stuff
extern arena omni_arena;

extern Camera2D camera;

extern arena tick_arena_raw[2];
extern size_t cur_tick_arena_idx;
extern arena *tick_arena;

extern u32 time_ms;
extern u32 time_delta_ms;

extern u32 tick_cnt;
extern u32 tick_delta_ms;
extern f32 tick_frame_alpha;
extern f32 tick_accumulate_time_ms;
