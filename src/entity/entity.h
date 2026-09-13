#pragma once

#include "action.h"
#include "collider.h"
#include "define.h"
#include "effect.h"
#include "sprite.h"

enum {
    _ENTITY_LOGIC_FLAG_MOVED
};
typedef struct {
    u32 pool_flag;
    u32 idx;

    u8 logic_flag;

    // sprite
    u32 sprite_begin;
    u32 sprite_len;

    // collider
    u32 collider_begin;
    u32 collider_len;

    // effect
    u32 effect_begin;
    u32 effect_len;

    // bounds
    f32 bounds_x, bounds_y, bounds_w, bounds_h;

    // chunk
    u16 chunk_x, chunk_y;
    u32 next_in_chunk, pre_in_chunk;
} entity;

typedef struct {
    f32 x[8], y[8], vx[8], vy[8];
} entity_pos_soa;

constexpr u32 ENTITY_CHUNK_SIZE = 512;
constexpr u32 ENTITY_CHUNK_COUNT = 64;

struct entity_sys {
    entity *entity_pool;
    entity_pos_soa *pos_soa_pool;
    usize entity_cap;
    usize entity_head;
    usize entity_max_idx;
    u32 entity_len;

    u32 chunk_begin_arr[ENTITY_CHUNK_COUNT * ENTITY_CHUNK_COUNT];
};
extern struct entity_sys entity_sys;

// =============================================================================
void entity_sys_init(usize cap);

// =============================================================================
u32 entity_create(f32 x, f32 y, entity **r_entity);
void entity_destroy(u32 idx);

[[nodiscard]] entity *entity_get(u32 idx);
[[nodiscard]] bool entity_alive(u32 idx);

u32 entity_add_sprite(u32 idx, u32 profile_idx, sprite **r_sprite);
u32 entity_add_collider(u32 idx, collider **r_collider);

u32 entity_add_effect(u32 idx, u32 effect_type, effect **r_effect);

// =============================================================================
void entity_pos_set_position(u32 idx, f32 x, f32 y);
void entity_pos_set_velocity(u32 idx, f32 vx, f32 vy);

void entity_pos_get(u32 idx, f32 *x, f32 *y, f32 *vx, f32 *vy);

// =============================================================================
void entity_query(f32 x, f32 y, f32 w, f32 h, u32 **r_entity_arr, usize *r_entity_arr_len);

// =============================================================================
void entity_sys_update();
