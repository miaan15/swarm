#include "entity.h"

#include "context.h"
#include "log.h"
#include <assert.h>

struct entity_sys entity_sys = {0};

// =============================================================================
void entity_sys_init(usize cap) {
    entity_sys.entity_pool = arena_alloc(&omni_arena, cap * sizeof(entity));
    memset(entity_sys.entity_pool, 0, cap * sizeof(entity));

    entity_sys.pos_pool = arena_alloc(&omni_arena, ((cap + 7) / 8) * sizeof(entity_pos_soa));
    memset(entity_sys.pos_pool, 0, cap * sizeof(entity));

    entity_sys.cap = cap;

    // stub
    entity_sys.head = entity_sys.max_idx = entity_sys.len = 1;
}

// =============================================================================
u32 entity_create(f32 x, f32 y, entity **r_entity) {
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

    entity_pos_set_position(idx, x, y);

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

    //
    entity_pos_set_velocity(idx, 0, 0);

    // destroy all sprites
    for (u32 sprite_idx = ett->sprite_begin; sprite_idx != 0;) {
        sprite *spr = sprite_get(sprite_idx);
        assert(spr->entity_idx == idx);

        sprite_destroy(sprite_idx);

        sprite_idx = spr->next_sprite;
    }

    // destroy all colliders
    for (u32 collider_idx = ett->collider_begin; collider_idx != 0;) {
        collider *col = collider_get(collider_idx);
        assert(col->entity_idx == idx);

        collider_destroy(collider_idx);

        collider_idx = col->next_collider;
    }

    // destroy all effects
    for (u32 effect_idx = ett->effect_begin; effect_idx != 0;) {
        effect *col = effect_get(effect_idx);
        assert(col->entity_idx == idx);

        effect_destroy(effect_idx);

        effect_idx = col->next_effect;
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
void entity_pos_set_position(u32 idx, f32 x, f32 y) {
    if (idx == 0 || idx >= entity_sys.max_idx) {
        log_err("entity_pos_set_position(): Entity [%u] invalid", idx);
        assert(false);
        return;
    }
    usize i = idx / 8;
    usize j = idx % 8;
    entity_sys.pos_pool[i].x[j] = x;
    entity_sys.pos_pool[i].y[j] = y;

    entity_sys.entity_pool[idx].logic_flag |= (1 << _ENTITY_LOGIC_FLAG_MOVED);
}

void entity_pos_set_velocity(u32 idx, f32 vx, f32 vy) {
    if (idx == 0 || idx >= entity_sys.max_idx) {
        log_err("entity_pos_set_velocity(): Entity [%u] invalid", idx);
        assert(false);
        return;
    }
    usize i = idx / 8;
    usize j = idx % 8;
    entity_sys.pos_pool[i].vx[j] = vx;
    entity_sys.pos_pool[i].vy[j] = vy;
}

entity_pos entity_pos_get(u32 idx) {
    if (idx == 0 || idx >= entity_sys.max_idx) {
        log_err("entity_pos_get(): Entity [%u] invalid => stub => stub", idx);
        assert(false);
        return (entity_pos){0};
    }
    usize i = idx / 8;
    usize j = idx % 8;
    entity_pos_soa *soa = &entity_sys.pos_pool[i];
    return (entity_pos){ soa->x[j], soa->y[j], soa->vx[j], soa->vy[j] };
}

// =============================================================================
void entity_sys_update() {
    // handle
    for (usize i = 1; i < entity_sys.max_idx; ++i) {
        entity *ett = entity_get(i);
        if (ett->pool_flag != ALIVE_POOL_FLAG) continue;

        fn_handle_entity(ett);
    }

    // update position
    const f32 dt = tick_delta_ms;
    for (usize i = 0; i < (entity_sys.max_idx + 7) / 8; ++i) {
        for (usize j = 0; j < 8; ++j) {
            entity_sys.pos_pool[i].x[j] += entity_sys.pos_pool[i].vx[j] * dt;
            entity_sys.pos_pool[i].y[j] += entity_sys.pos_pool[i].vy[j] * dt;
        }
    }

    // update components
    for (usize i = 1; i < entity_sys.max_idx; ++i) {
        entity *ett = entity_get(i);
        if (ett->pool_flag != ALIVE_POOL_FLAG) continue;

        usize ii = i / 8;
        usize ij = i % 8;
        entity_pos_soa *soa = &entity_sys.pos_pool[ii];

        if (((ett->logic_flag >> _ENTITY_LOGIC_FLAG_MOVED) & 1)
            || soa->vx[ij] != 0 || soa->vy[ij] != 0) {
            // update all sprites
            for (u32 sprite_idx = ett->sprite_begin; sprite_idx != 0;) {
                sprite *spr = sprite_get(sprite_idx);
                assert(spr->entity_idx == i);

                spr->x = soa->x[ij] + spr->offset_x;
                spr->y = soa->y[ij] + spr->offset_y;

                sprite_idx = spr->next_sprite;
            }

            // update all colliders
            for (u32 collider_idx = ett->collider_begin; collider_idx != 0;) {
                collider *spr = collider_get(collider_idx);
                assert(spr->entity_idx == i);

                spr->x = soa->x[ij] + spr->offset_x;
                spr->y = soa->y[ij] + spr->offset_y;

                collider_idx = spr->next_collider;
            }

            ett->logic_flag &= ~(1 << _ENTITY_LOGIC_FLAG_MOVED);
        }
    }
}
