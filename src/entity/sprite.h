#pragma once

#include "define.h"

typedef struct {
    u32 tex;
    f32 x, y, w, h;
} sprite_profile;

typedef struct {
    u32 pool_flag;
    u32 idx;

    u32 tex;
    f32 sx, sy, sw, sh;
    f32 x, y, w, h;
    i8 z;

    bool show;

    // entity
    u32 entity_idx;
    u32 next_in_entity;

    f32 offset_x, offset_y;

    // chunk
    i32 chunk_x, chunk_y;
    u32 next_in_chunk, pre_in_chunk;
} sprite;

struct sprite_sys {
    sprite_profile *profile_arr;
    usize profile_cap;
    usize profile_len;

    sprite *sprite_pool;
    usize sprite_cap;
    usize sprite_head;
    usize sprite_max_idx;
    u32 sprite_len;
};
extern struct sprite_sys sprite_sys;

// =============================================================================
void sprite_sys_init(usize profile_cap, usize sprite_cap);

// =============================================================================
u32 sprite_profile_create(u32 tex, f32 x, f32 y, f32 w, f32 h);
[[nodiscard]] sprite_profile *sprite_profile_get(usize idx);

// =============================================================================
u32 sprite_create(u32 profile_idx, sprite **r_sprite);
void sprite_destroy(u32 idx);

[[nodiscard]] sprite *sprite_get(u32 idx);

// =============================================================================
void sprite_sys_draw();
