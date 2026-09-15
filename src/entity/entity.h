#pragma once

#include "collider.h"
#include "define.h"
#include "status.h"
#include "portrait.h"

enum {
    _ENTITY_LOGIC_FLAG_CREATED,
    _ENTITY_LOGIC_FLAG_MOVED,
};
typedef struct {
    u32 pool_flag;
    u32 idx;

    u8 logic_flag;

    f32 scale_x, scale_y;
    i8 z;

    // portrait
    u32 portrait_begin;
    u32 portrait_len;

    // collider
    u32 collider_begin;
    u32 collider_len;

    // status
    u32 status_begin;
    u32 status_len;

    // chunk
    i32 chunk_x, chunk_y;
    u32 next_in_chunk, pre_in_chunk;
} entity;

typedef struct {
    f32 x[8], y[8], vx[8], vy[8];
} entity_pos_soa;

struct entity_sys {
    entity *entity_pool;
    entity_pos_soa *pos_soa_pool;
    usize entity_cap;
    usize entity_head;
    usize entity_max_idx;
    u32 entity_len;
};
extern struct entity_sys entity_sys;

// =============================================================================
void entity_sys_init(usize cap);

// =============================================================================
u32 entity_create(f32 x, f32 y, entity **r_entity);
void entity_destroy(u32 idx);

[[nodiscard]] entity *entity_get(u32 idx);
[[nodiscard]] bool entity_alive(u32 idx);

u32 entity_add_portrait(u32 idx, u32 profile_idx, portrait **r_portrait);
u32 entity_add_collider(u32 idx, f32 w, f32 h, collider **r_collider);

u32 entity_add_status(u32 idx, u32 status_type, status **r_status);

// =============================================================================
void entity_pos_set_position(u32 idx, f32 x, f32 y);
void entity_pos_set_velocity(u32 idx, f32 vx, f32 vy);

void entity_pos_get(u32 idx, f32 *x, f32 *y, f32 *vx, f32 *vy);

// =============================================================================
void entity_sys_update();
