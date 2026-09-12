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
        log_err("entity_create(): too much entities (%zu) => stub", entity_sys.len);
        assert(false);
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
    ett->idx = idx;

    log_debug("Created Entity [%u]", idx);

    if (r_entity != nullptr) *r_entity = ett;
    return idx;
}

void entity_destroy(u32 idx) {
    entity *ett = &entity_sys.entity_pool[idx];

    if (ett->pool_flag != ALIVE_POOL_FLAG) {
        log_warn("entity_destroy(): Entity [%u] already dead", idx);
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

        sprite_idx = spr->next_sprite;
    }

    // destroy all collideres
    for (u32 collider_idx = ett->collider_begin; collider_idx != 0;) {
        collider *col = collider_get(collider_idx);
        assert(col->entity_idx == idx);

        collider_destroy(collider_idx);

        collider_idx = col->next_collider;
    }

    log_debug("Destroyed Entity [%u]", idx);
}

entity *entity_get(u32 idx) {
    if (idx == 0 || idx >= entity_sys.max_idx) {
        log_err("entity_get(): Entity [%u] invalid => stub", idx);
        assert(false);
        return &entity_sys.entity_pool[0];
    }
    return &entity_sys.entity_pool[idx];
}

bool entity_alive(u32 idx) {
    if (idx == 0 || idx >= entity_sys.max_idx) {
        return false;
    }
    return entity_sys.entity_pool[idx].pool_flag == ALIVE_POOL_FLAG;
}

u32 entity_add_sprite(u32 idx, u32 profile_idx, sprite **r_sprite) {
    if (idx == 0 || idx >= entity_sys.max_idx) {
        log_err("entity_add_sprite(): Entity [%u] invalid => stub", idx);
        assert(false);
        return 0;
    }

    u32 spr_idx = sprite_create(profile_idx, r_sprite);

    entity *ett = &entity_sys.entity_pool[idx];
    (*r_sprite)->next_sprite = ett->sprite_begin;
    ett->sprite_begin = spr_idx;

    (*r_sprite)->entity_idx = idx;

    log_debug("Added Sprite [%u] to Entity [%u]", spr_idx, idx);

    return spr_idx;
}

u32 entity_add_collider(u32 idx, collider **r_collider) {
    if (idx == 0 || idx >= entity_sys.max_idx) {
        log_err("entity_add_collider(): Entity [%u] invalid => stub", idx);
        assert(false);
        return 0;
    }

    u32 col_idx = collider_create(r_collider);
    collider_add_to_tree(col_idx);

    entity *ett = &entity_sys.entity_pool[idx];
    (*r_collider)->next_collider = ett->collider_begin;
    ett->collider_begin = col_idx;

    (*r_collider)->entity_idx = idx;

    log_debug("Added Collider [%u] to Entity [%u]", col_idx, idx);

    return col_idx;
}

u32 entity_add_effect(u32 idx, u32 effect_type, effect **r_effect) {
    if (idx == 0 || idx >= entity_sys.max_idx) {
        log_err("entity_add_effect(): Entity [%u] invalid => stub", idx);
        assert(false);
        return 0;
    }

    u32 eff_idx = effect_create(effect_type, r_effect);

    entity *ett = &entity_sys.entity_pool[idx];
    (*r_effect)->next_effect = ett->effect_begin;
    ett->effect_begin = eff_idx;

    (*r_effect)->entity_idx = idx;

    log_debug("Added Effect [%u] to Entity [%u]", eff_idx, idx);

    return eff_idx;
}

// =============================================================================
void entity_sys_update() {
    for (usize i = 1; i < entity_sys.max_idx; ++i) {
        entity *ett = entity_get(i);
        if (ett->pool_flag != ALIVE_POOL_FLAG) continue;

        fn_handle_entity(ett);
    }
}
