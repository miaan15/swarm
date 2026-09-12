#pragma once

#include "define.h"

// TODO use QuadTree to culling
typedef struct {
    u32 tex;
    f32 x, y, w, h;
} sprite_profile;

typedef struct {
    u32 pool_flag;
    u32 pool_idx;

    u32 profile_idx;
    f32 x, y;
    i8 z;

    // for entity stuff
    u32 entity_idx;
    u32 next_sprite;
    f32 offset_x, offset_y;
} sprite;

struct sprite_sys {
    sprite_profile *prf_arr;
    usize prf_cap;
    usize prf_len;

    sprite *sprite_pool;
    usize cap;
    usize head;
    usize max_idx;
    u32 len;
};
extern struct sprite_sys sprite_sys;

// =============================================================================
void sprite_sys_init(usize profile_cap, usize sprite_cap);

// =============================================================================
u32 sprite_profile_create(u32 tex, f32 x, f32 y, f32 w, f32 h);
[[nodiscard]] sprite_profile sprite_profile_get(usize idx);

// =============================================================================
u32 sprite_create(u32 profile_idx, sprite **r_sprite);
void sprite_destroy(u32 idx);

[[nodiscard]] sprite *sprite_get(usize idx);

// =============================================================================
void sprite_sys_draw();
