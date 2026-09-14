#include "entity.h"

#include "context.h"
#include "log.h"
#include <assert.h>

struct entity_sys entity_sys = {0};

// =============================================================================
void entity_sys_init(usize cap) {
    entity_sys.entity_pool = arena_alloc(&omni_arena, cap * sizeof(entity));
    entity_sys.pos_soa_pool = arena_alloc_aligned(&omni_arena, ((cap + 7) / 8) * sizeof(entity_pos_soa), 32);

    entity_sys.entity_cap = cap;

    // stub
    entity_sys.entity_head = entity_sys.entity_max_idx = entity_sys.entity_len = 1;
}

// =============================================================================
u32 entity_create(f32 x, f32 y, entity **r_entity) {
    if (entity_sys.entity_len >= entity_sys.entity_cap) {
        log_err("entity_create(): too much entities (%zu) => stub", entity_sys.entity_len);
        assert(false);
        return 0;
    }

    usize idx = entity_sys.entity_head;
    entity *ett = &entity_sys.entity_pool[idx];

    if (idx == entity_sys.entity_max_idx) {
        ++entity_sys.entity_max_idx;
        ++entity_sys.entity_head;
    } else {
        entity_sys.entity_head = ett->pool_flag;
    }

    ++entity_sys.entity_len;

    // setup entity
    memset(ett, 0, sizeof(entity));
    ett->pool_flag = ALIVE_POOL_FLAG;
    ett->idx = idx;

    ett->logic_flag |= (1 << _ENTITY_LOGIC_FLAG_CREATED);

    entity_pos_set_position(idx, x, y);
    entity_pos_set_velocity(idx, 0, 0);

    chunk_add_entity(idx);

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

    ett->pool_flag = entity_sys.entity_head;
    entity_sys.entity_head = idx;

    --entity_sys.entity_len;

    //
    entity_pos_set_velocity(idx, 0, 0);

    // destroy all portraits
    for (u32 portrait_idx = ett->portrait_begin; portrait_idx != 0;) {
        portrait *potr = portrait_get(portrait_idx);
        assert(potr->entity_idx == idx);

        portrait_destroy(portrait_idx);

        portrait_idx = potr->next_in_entity;
    }

    // destroy all colliders
    for (u32 collider_idx = ett->collider_begin; collider_idx != 0;) {
        collider *col = collider_get(collider_idx);
        assert(col->entity_idx == idx);

        collider_destroy(collider_idx);

        collider_idx = col->next_collider;
    }

    // destroy all statuses
    for (u32 stt_idx = ett->status_begin; stt_idx != 0;) {
        status *stt = status_get(stt_idx);
        assert(stt->entity_idx == idx);

        status_destroy(stt_idx);

        stt_idx = stt->next_in_entity;
    }

    chunk_remv_entity(idx);

    log_debug("Destroyed Entity [%u]", idx);
}

entity *entity_get(u32 idx) {
    if (idx == 0 || idx >= entity_sys.entity_max_idx) {
        log_err("entity_get(): Entity [%u] invalid => stub", idx);
        assert(false);
        return &entity_sys.entity_pool[0];
    }
    return &entity_sys.entity_pool[idx];
}

bool entity_alive(u32 idx) {
    if (idx == 0 || idx >= entity_sys.entity_max_idx) {
        return false;
    }
    return entity_sys.entity_pool[idx].pool_flag == ALIVE_POOL_FLAG;
}

u32 entity_add_portrait(u32 idx, u32 profile_idx, portrait **r_portrait) {
    if (idx == 0 || idx >= entity_sys.entity_max_idx) {
        log_err("entity_add_portrait(): Entity [%u] invalid => stub", idx);
        assert(false);
        return 0;
    }

    u32 potr_idx = portrait_create(profile_idx, r_portrait);

    entity *ett = &entity_sys.entity_pool[idx];
    (*r_portrait)->next_in_entity = ett->portrait_begin;
    ett->portrait_begin = potr_idx;

    (*r_portrait)->entity_idx = idx;

    log_debug("Added Portrait [%u] to Entity [%u]", potr_idx, idx);

    return potr_idx;
}

u32 entity_add_collider(u32 idx, collider **r_collider) {
    if (idx == 0 || idx >= entity_sys.entity_max_idx) {
        log_err("entity_add_collider(): Entity [%u] invalid => stub", idx);
        assert(false);
        return 0;
    }

    u32 col_idx = collider_create(r_collider);

    entity *ett = &entity_sys.entity_pool[idx];
    (*r_collider)->next_collider = ett->collider_begin;
    ett->collider_begin = col_idx;

    (*r_collider)->entity_idx = idx;

    log_debug("Added Collider [%u] to Entity [%u]", col_idx, idx);

    return col_idx;
}

u32 entity_add_status(u32 idx, u32 status_type, status **r_status) {
    if (idx == 0 || idx >= entity_sys.entity_max_idx) {
        log_err("entity_add_status(): Entity [%u] invalid => stub", idx);
        assert(false);
        return 0;
    }

    u32 stt_idx = status_create(status_type, r_status);

    entity *ett = &entity_sys.entity_pool[idx];
    (*r_status)->next_in_entity = ett->status_begin;
    ett->status_begin = stt_idx;

    (*r_status)->entity_idx = idx;

    log_debug("Added Status [%u] to Entity [%u]", stt_idx, idx);

    return stt_idx;
}

// =============================================================================
void entity_pos_set_position(u32 idx, f32 x, f32 y) {
    if (idx == 0 || idx >= entity_sys.entity_max_idx) {
        log_err("entity_pos_set_position(): Entity [%u] invalid", idx);
        assert(false);
        return;
    }
    usize i = idx / 8;
    usize j = idx % 8;
    entity_sys.pos_soa_pool[i].x[j] = x;
    entity_sys.pos_soa_pool[i].y[j] = y;

    entity_sys.entity_pool[idx].logic_flag |= (1 << _ENTITY_LOGIC_FLAG_MOVED);
}

void entity_pos_set_velocity(u32 idx, f32 vx, f32 vy) {
    if (idx == 0 || idx >= entity_sys.entity_max_idx) {
        log_err("entity_pos_set_velocity(): Entity [%u] invalid", idx);
        assert(false);
        return;
    }
    usize i = idx / 8;
    usize j = idx % 8;
    entity_sys.pos_soa_pool[i].vx[j] = vx;
    entity_sys.pos_soa_pool[i].vy[j] = vy;
}

void entity_pos_get(u32 idx, f32 *x, f32 *y, f32 *vx, f32 *vy) {
    if (idx == 0 || idx >= entity_sys.entity_max_idx) {
        log_err("entity_pos_get(): Entity [%u] invalid", idx);
        assert(false);
        return;
    }
    usize i = idx / 8;
    usize j = idx % 8;
    entity_pos_soa *soa = &entity_sys.pos_soa_pool[i];
    if (x != nullptr) *x = soa->x[j];
    if (y != nullptr) *y = soa->y[j];
    if (vx != nullptr) *vx = soa->vx[j];
    if (vy != nullptr) *vy = soa->vy[j];
}

// =============================================================================
void entity_sys_update() {
    // handle
    for (usize i = 1; i < entity_sys.entity_max_idx; ++i) {
        entity *ett = entity_get(i);
        if (ett->pool_flag != ALIVE_POOL_FLAG) continue;

        fn_handle_entity(ett);
    }

    SET_CLOCK(CLOCK_START_ENTITY_POS_UPDATE);
    // update position - hot af line
    const f32 dt = (f32)tick_delta_ms / 1000.0f;
    for (usize i = 0; i < (entity_sys.entity_max_idx + 7) / 8; ++i) {
        for (usize j = 0; j < 8; ++j) {
            entity_sys.pos_soa_pool[i].x[j] += entity_sys.pos_soa_pool[i].vx[j] * dt;
            entity_sys.pos_soa_pool[i].y[j] += entity_sys.pos_soa_pool[i].vy[j] * dt;
        }
    }
    SET_CLOCK(CLOCK_END_ENTITY_POS_UPDATE);

    SET_CLOCK(CLOCK_START_ENTITY_COMPS_UPDATE);
    // update components
    for (usize i = 1; i < entity_sys.entity_max_idx; ++i) {
        entity *ett = entity_get(i);
        if (ett->pool_flag != ALIVE_POOL_FLAG) continue;

        usize ii = i / 8;
        usize ij = i % 8;
        entity_pos_soa *soa = &entity_sys.pos_soa_pool[ii];

        // if the entity moved
        if (((ett->logic_flag >> _ENTITY_LOGIC_FLAG_MOVED) & 1)
            || ((ett->logic_flag >> _ENTITY_LOGIC_FLAG_CREATED) & 1)
            || soa->vx[ij] != 0 || soa->vy[ij] != 0) {

            f32 x = soa->x[ij], y = soa->y[ij];

            // update all portraits
            for (u32 portrait_idx = ett->portrait_begin; portrait_idx != 0;) {
                portrait *potr = portrait_get(portrait_idx);
                assert(potr->entity_idx == i);

                potr->x = x + potr->offset_x;
                potr->y = y + potr->offset_y;

                portrait_idx = potr->next_in_entity;
            }

            // update all colliders
            for (u32 collider_idx = ett->collider_begin; collider_idx != 0;) {
                collider *col = collider_get(collider_idx);
                assert(col->entity_idx == i);

                col->x = x + col->offset_x;
                col->y = y + col->offset_y;

                collider_idx = col->next_collider;
            }

            // update chunk
            chunk_update_entity(i);

            // de-flag
            ett->logic_flag &= ~(1 << _ENTITY_LOGIC_FLAG_CREATED);
            ett->logic_flag &= ~(1 << _ENTITY_LOGIC_FLAG_MOVED);
        }
    }
    SET_CLOCK(CLOCK_END_ENTITY_COMPS_UPDATE);
}
