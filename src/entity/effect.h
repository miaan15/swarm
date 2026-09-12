#pragma once

#include "define.h"

typedef struct {
    u32 pool_flag;
    u32 pool_idx;

    u32 type;

    // entity stuff
    u32 entity_idx;
    u32 next_effect;

    // stats
    u32 stack_count;

    f32 duration;
    f32 last_check_time;

    // more specific stats
    f32 damage;
} effect;

typedef struct {
} effect_entity_owned;

struct effect_sys {
    effect *effect_pool;
    usize cap;
    usize head;
    usize max_idx;
    u32 len;
};
extern struct effect_sys effect_sys;

// =============================================================================
void effect_sys_init(usize cap);

// =============================================================================
u32 effect_create(u32 type, effect **r_effect);
void effect_destroy(u32 idx);

[[nodiscard]] effect *effect_get(u32 idx);

// =============================================================================
void effect_sys_update();
