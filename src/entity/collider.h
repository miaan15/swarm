#pragma once

#include "define.h"

typedef struct {
    u32 pool_flag;
    u32 idx;

    f32 sw, sh;
    f32 x, y, w, h;

    // entity
    u32 entity_idx;
    u32 next_collider;

    f32 offset_x, offset_y;

    // chunk
    i32 chunk_x, chunk_y;
    u32 next_in_chunk, pre_in_chunk;
} collider;

struct collider_sys {
    // collider pool
    collider *collider_pool;
    usize collider_cap;
    usize collider_head;
    usize collider_max_idx;
    u32 collider_len;
};
extern struct collider_sys collider_sys;

// =============================================================================
void collider_sys_init(usize cap);

// =============================================================================
u32 collider_create(f32 w, f32 h, collider **r_collider);
void collider_destroy(u32 idx);

[[nodiscard]] collider *collider_get(u32 idx);

// =============================================================================
void collider_sys_update();
