#include "entity.h"

#include "context.h"
#include "log.h"
#include "sprite.h"
#include <assert.h>

#define ALIVE_POOL_FLAG ((u32)-1)

struct entity_sys entity_sys = {0};

void entity_sys_init(usize cap) {
    entity_sys.entity_pool = arena_alloc(&omni_arena, cap * sizeof(entity));
    entity_sys.cap = cap;
    memset(entity_sys.entity_pool, 0, cap * sizeof(entity));

    // stub
    entity_sys.head = entity_sys.max_idx = entity_sys.len = 1;
}

u32 entity_create() {
    if (entity_sys.len >= entity_sys.cap) {
        log_err("entity_create(): too much entities => stub");
        return 0;
    }

    usize idx = entity_sys.head;
    entity *ett = &entity_sys.entity_pool[idx];

    if (idx == entity_sys.max_idx) {
        ++entity_sys.max_idx;
        ++entity_sys.head;
    } else {
        entity_sys.head = ett->pool_flag;
    }

    ++entity_sys.len;

    memset(ett, 0, sizeof(entity));
    ett->pool_flag = ALIVE_POOL_FLAG;

    log_debug("Created Entity [%u]", idx);

    return idx;
}

void entity_destroy(u32 idx) {
    entity *ett = &entity_sys.entity_pool[idx];

    if (ett->pool_flag != ALIVE_POOL_FLAG) {
        log_warn("entity_destroy(): entity already dead");
        return;
    }

    ett->pool_flag = entity_sys.head;
    entity_sys.head = idx;

    --entity_sys.len;

    // destroy all sprites
    for (u32 sprite_idx = ett->sprite_begin; sprite_idx != 0;) {
        sprite *spr = sprite_get(sprite_idx);
        assert(spr->entity_idx == idx);

        sprite_destroy(sprite_idx);

        sprite_idx = spr->next;
    }

    log_debug("Destroyed Entity [%u]", idx);
}

entity *entity_get(usize idx) {
    return &entity_sys.entity_pool[idx];
}
