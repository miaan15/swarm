#pragma once

#include "define.h"
#include <raymath.h>

typedef struct {
    u32 pool_flag;

    Vector2 position;
} entity;

struct entity_sys {
    entity *entity_pool;
    usize cap;
    usize head;
    usize max_idx;
    usize len;
};
extern struct entity_sys entity_sys;

void entity_sys_init(usize cap);

entity *entity_create();
void entity_destroy(entity *ett);
