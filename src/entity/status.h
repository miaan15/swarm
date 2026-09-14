#pragma once

#include "define.h"

typedef struct {
    u32 pool_flag;
    u32 pool_idx;

    u32 type;

    // entity stuff
    u32 entity_idx;
    u32 next_in_entity;

    // stats
    u32 stack_count;

    f32 duration;
    f32 last_check_time;

    // more specific stats
    f32 damage;
} status;

struct status_sys {
    status *status_pool;
    usize status_cap;
    usize status_head;
    usize status_max_idx;
    u32 status_len;
};
extern struct status_sys status_sys;

// =============================================================================
void status_sys_init(usize cap);

// =============================================================================
u32 status_create(u32 type, status **r_status);
void status_destroy(u32 idx);

[[nodiscard]] status *status_get(u32 idx);

// =============================================================================
void status_sys_update();
