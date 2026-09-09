#pragma once

#include "box.h"
#include "define.h"
#include "sprite.h"
#include <raymath.h>

typedef struct {
    u32 pool_flag;

    Vector2 position;

    u32 sprite_begin;
    u32 sprite_len;

    u32 box_begin;
    u32 box_len;
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

[[nodiscard]] entity *entity_get(usize idx);

u32 entity_add_sprite(u32 idx, u32 prf_idx, sprite **r_sprite);
u32 entity_add_box(u32 idx, box **r_box);
