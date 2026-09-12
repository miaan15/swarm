#pragma once

#include "action.h"
#include "collider.h"
#include "define.h"
#include "effect.h"
#include "sprite.h"

typedef struct {
    u32 pool_flag;
    u32 idx;

    u32 sprite_begin;
    u32 sprite_len;

    u32 collider_begin;
    u32 collider_len;

    u32 effect_begin;
    u32 effect_len;
} entity;

struct entity_sys {
    entity *entity_pool;
    usize cap;
    usize head;
    usize max_idx;
    u32 len;
};
extern struct entity_sys entity_sys;

// =============================================================================
void entity_sys_init(usize cap);

// =============================================================================
u32 entity_create(entity **r_entity);
void entity_destroy(u32 idx);

[[nodiscard]] entity *entity_get(u32 idx);
[[nodiscard]] bool entity_alive(u32 idx);

u32 entity_add_sprite(u32 idx, u32 profile_idx, sprite **r_sprite);
u32 entity_add_collider(u32 idx, collider **r_collider);

u32 entity_add_effect(u32 idx, u32 effect_type, effect **r_effect);

// =============================================================================
void entity_sys_update();
