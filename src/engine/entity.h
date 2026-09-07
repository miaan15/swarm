#pragma once

#include "define.h"
#include <raymath.h>

typedef struct {
    u32 pool_flag;

    Vector2 position;

    u32 sprite_begin;
    u32 sprite_len;
} entity;

struct entity_sys {
    entity *entity_pool;
    usize cap;
    usize head;
    usize max_idx;
    u32 len;
};
extern struct entity_sys entity_sys;

void entity_sys_init(usize cap);

u32 entity_create();
void entity_destroy(u32 idx);
entity *entity_get(usize idx);
