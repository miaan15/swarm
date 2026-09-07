#include "entity.h"

#include "context.h"
#include "log.h"
#include <assert.h>

#define ALIVE_POOL_FLAG ((u32)-1)

struct entity_sys entity_sys = {0};

void entity_sys_init(usize cap) {
    entity_sys.entity_pool = arena_alloc(&omni_arena, cap * sizeof(entity), alignof(entity));
    entity_sys.cap = cap;
    memset(entity_sys.entity_pool, 0, cap * sizeof(entity));

    // stub
    entity_sys.head = entity_sys.max_idx = entity_sys.len = 1;
}

entity *entity_create() {
    if (entity_sys.len >= entity_sys.cap) {
        log_err("entity_create(): too much entities => stub");
        return &entity_sys.entity_pool[0];
    }

    usize idx = entity_sys.head;
    if (idx == entity_sys.max_idx) {
        ++entity_sys.max_idx;
        ++entity_sys.head;
    } else {
        assert(idx < entity_sys.max_idx);
        entity_sys.head = entity_sys.entity_pool[idx].pool_flag;
    }

    entity *ett = &entity_sys.entity_pool[idx];
    memset(ett, 0, sizeof(entity));
    ett->pool_flag = ALIVE_POOL_FLAG;

    log_debug("Created Entity [%zu]", idx);

    return ett;
}

void entity_destroy(entity *ett) {
    if (ett->pool_flag != ALIVE_POOL_FLAG) {
        log_warn("entity_destroy(): entity already dead");
        return;
    }

    usize idx = ett - entity_sys.entity_pool;

    ett->pool_flag = entity_sys.head;
    entity_sys.head = idx;

    --entity_sys.len;

    // TODO destroy what that entity holds

    log_debug("Destroyed Entity [%zu]", idx);
}
