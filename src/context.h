#pragma once

#include "arena.h"
#include "define.h"
#include "entity.h"

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

// functions binding
extern void (*fn_handle_entity)(entity *);
extern void (*fn_handle_action)(action *);
extern void (*fn_handle_effect)(effect *);

//
