#include "entity.h"

#include "context.h"
#include "log.h"
#include <assert.h>

struct entity_sys entity_sys = {0};

// =============================================================================
void entity_sys_init(usize cap) {
    entity_sys.entity_pool = arena_alloc(&omni_arena, cap * sizeof(entity));
    entity_sys.cap = cap;
    memset(entity_sys.entity_pool, 0, cap * sizeof(entity));

    // stub
    entity_sys.head = entity_sys.max_idx = entity_sys.len = 1;
}

// =============================================================================
u32 entity_create(entity **r_entity) {
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

    // setup entity
    memset(ett, 0, sizeof(entity));
    ett->pool_flag = ALIVE_POOL_FLAG;

    log_debug("Created Entity [%u]", idx);

    if (r_entity != nullptr) *r_entity = ett;
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

    // need to destroy "belongings"
    // destroy all sprites
    for (u32 sprite_idx = ett->sprite_begin; sprite_idx != 0;) {
        sprite *spr = sprite_get(sprite_idx);
        assert(spr->entity_idx == idx);

        sprite_destroy(sprite_idx);

        sprite_idx = spr->next;
    }

    // destroy all boxes
    for (u32 box_idx = ett->box_begin; box_idx != 0;) {
        box *bx = box_get(box_idx);
        assert(bx->entity_idx == idx);

        box_destroy(box_idx);

        box_idx = bx->next;
    }

    log_debug("Destroyed Entity [%u]", idx);
}

entity *entity_get(usize idx) {
    if (idx == 0 || idx >= entity_sys.max_idx) {
        log_err("entity_get(): entity invalid => stub");
        return &entity_sys.entity_pool[0];
    }
    return &entity_sys.entity_pool[idx];
}

u32 entity_add_sprite(u32 idx, u32 prf_idx, sprite **r_sprite) {
    u32 spr_idx = sprite_create(prf_idx, r_sprite);
    
    entity *ett = &entity_sys.entity_pool[idx];
    (*r_sprite)->next = ett->sprite_begin;
    ett->sprite_begin = spr_idx;

    (*r_sprite)->entity_idx = idx;

    return spr_idx;
}

u32 entity_add_box(u32 idx, box **r_box) {
    u32 bx_idx = box_create(r_box);
    
    entity *ett = &entity_sys.entity_pool[idx];
    (*r_box)->next = ett->box_begin;
    ett->box_begin = bx_idx;

    (*r_box)->entity_idx = idx;

    return bx_idx;
}
