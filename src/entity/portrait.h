#pragma once

#include "define.h"

typedef struct {
    u32 tex;
    f32 x, y, w, h;
} portrait_profile;

typedef struct {
    u32 pool_flag;
    u32 idx;

    u32 tex;
    f32 sx, sy, sw, sh;
    f32 x, y, w, h;

    // entity
    u32 entity_idx;
    u32 next_in_entity;

    f32 offset_x, offset_y;

    // chunk
    i32 chunk_x, chunk_y;
    u32 next_in_chunk, pre_in_chunk;
} portrait;

struct portrait_sys {
    portrait_profile *profile_arr;
    usize profile_cap;
    usize profile_len;

    portrait *portrait_pool;
    usize portrait_cap;
    usize portrait_head;
    usize portrait_max_idx;
    u32 portrait_len;
};
extern struct portrait_sys portrait_sys;

// =============================================================================
void portrait_sys_init(usize profile_cap, usize portrait_cap);

// =============================================================================
u32 portrait_profile_create(u32 tex, f32 x, f32 y, f32 w, f32 h);
[[nodiscard]] portrait_profile *portrait_profile_get(usize idx);

// =============================================================================
u32 portrait_create(u32 profile_idx, portrait **r_portrait);
void portrait_destroy(u32 idx);

[[nodiscard]] portrait *portrait_get(u32 idx);

// =============================================================================
void portrait_sys_update();
