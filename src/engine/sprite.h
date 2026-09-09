#pragma once

#include "define.h"

typedef struct {
    u32 tex;
    f32 x, y, w, h;
} sprite_prf;

typedef struct {
    u32 pool_flag;

    u32 prf_idx;
    f32 x, y; // become offset to entity of entity_idx != 0
    i8 z;

    // for entity stuff
    u32 entity_idx;
    u32 next;
} sprite;

struct sprite_sys {
    sprite_prf *prf_arr;
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
void sprite_sys_init(usize prf_cap, usize sprite_cap);

// =============================================================================
u32 sprite_prf_make(u32 tex, f32 x, f32 y, f32 w, f32 h);
[[nodiscard]] sprite_prf sprite_prf_get(usize idx);

// =============================================================================
u32 sprite_create(u32 prf_idx);
void sprite_destroy(u32 idx);

[[nodiscard]] sprite *sprite_get(usize idx);

// =============================================================================
void sprite_sys_draw();
