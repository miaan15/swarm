#include "entity.h"

#include "context.h"
#include "log.h"
#include <assert.h>
#include <math.h>

struct entity_sys entity_sys = {0};

// =============================================================================
void entity_cal_chunk_pos(f32 x, f32 y, u16 *chunk_x, u16 *chunk_y);
void entity_chunk_add(u32 ett_idx);
void entity_chunk_remv(u32 ett_idx);

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

    entity_chunk_add(idx);

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

    // destroy all sprites
    for (u32 sprite_idx = ett->sprite_begin; sprite_idx != 0;) {
        sprite *spr = sprite_get(sprite_idx);
        assert(spr->entity_idx == idx);

        sprite_destroy(sprite_idx);

        sprite_idx = spr->next_in_entity;
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

        effect_idx = col->next_in_entity;
    }

    entity_chunk_remv(idx);

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

u32 entity_add_sprite(u32 idx, u32 profile_idx, sprite **r_sprite) {
    if (idx == 0 || idx >= entity_sys.entity_max_idx) {
        log_err("entity_add_sprite(): Entity [%u] invalid => stub", idx);
        assert(false);
        return 0;
    }

    u32 spr_idx = sprite_create(profile_idx, r_sprite);

    entity *ett = &entity_sys.entity_pool[idx];
    (*r_sprite)->next_in_entity = ett->sprite_begin;
    ett->sprite_begin = spr_idx;

    (*r_sprite)->entity_idx = idx;

    log_debug("Added Sprite [%u] to Entity [%u]", spr_idx, idx);

    return spr_idx;
}

u32 entity_add_collider(u32 idx, collider **r_collider) {
    if (idx == 0 || idx >= entity_sys.entity_max_idx) {
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
    if (idx == 0 || idx >= entity_sys.entity_max_idx) {
        log_err("entity_add_effect(): Entity [%u] invalid => stub", idx);
        assert(false);
        return 0;
    }

    u32 eff_idx = effect_create(effect_type, r_effect);

    entity *ett = &entity_sys.entity_pool[idx];
    (*r_effect)->next_in_entity = ett->effect_begin;
    ett->effect_begin = eff_idx;

    (*r_effect)->entity_idx = idx;

    log_debug("Added Effect [%u] to Entity [%u]", eff_idx, idx);

    return eff_idx;
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
void entity_query(f32 x, f32 y, f32 w, f32 h, u32 **r_entity_arr, usize *r_entity_arr_len) {
    *r_entity_arr = nullptr; *r_entity_arr_len = 0;
    usize entity_arr_cap = 0;

    f32 min_x = x - (f32)ENTITY_CHUNK_SIZE / 4;
    f32 min_y = y - (f32)ENTITY_CHUNK_SIZE / 4;
    f32 max_x = x + w + (f32)ENTITY_CHUNK_SIZE / 4;
    f32 max_y = y + h + (f32)ENTITY_CHUNK_SIZE / 4;

    u16 start_cx, end_cx, start_cy, end_cy;
    entity_cal_chunk_pos(min_x, min_y, &start_cx, &start_cy);
    entity_cal_chunk_pos(max_x, max_y, &end_cx, &end_cy);

    for (u16 j = start_cy; j <= end_cy; ++j) {
        for (u16 i = start_cx; i <= end_cx; ++i) {
            u32 ett_idx = entity_sys.chunk_begin_arr[i + j * ENTITY_CHUNK_COUNT];
            while (ett_idx != 0) {
                assert(ett_idx < entity_sys.entity_max_idx);
                entity *ett = &entity_sys.entity_pool[ett_idx];

                // if bounds collide
                if (ett->bounds_x < x + w && ett->bounds_x + ett->bounds_w > x
                    && ett->bounds_y < y + h && ett->bounds_y + ett->bounds_h > y) {
                    // push to result
                    if (*r_entity_arr_len >= entity_arr_cap) {
                        entity_arr_cap = entity_arr_cap < 32 ? 32 : entity_arr_cap * 2;
                        u32 *_new = arena_alloc(tick_arena, entity_arr_cap * sizeof(u32));
                        if (*r_entity_arr != nullptr) {
                            memcpy(_new, *r_entity_arr, *r_entity_arr_len * sizeof(u32));
                        }
                        *r_entity_arr = _new;
                    }

                    (*r_entity_arr)[(*r_entity_arr_len)++] = ett_idx;
                }

                ett_idx = ett->next_in_chunk;
            }
        }
    }
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

        // sneak in some bounds update
        f32 min_x = INFINITY, min_y = INFINITY, max_x = -INFINITY, max_y = -INFINITY;

        // if the entity moved
        if (((ett->logic_flag >> _ENTITY_LOGIC_FLAG_MOVED) & 1)
            || ((ett->logic_flag >> _ENTITY_LOGIC_FLAG_CREATED) & 1)
            || soa->vx[ij] != 0 || soa->vy[ij] != 0) {

            f32 x = soa->x[ij], y = soa->y[ij];

            // update all sprites
            for (u32 sprite_idx = ett->sprite_begin; sprite_idx != 0;) {
                sprite *spr = sprite_get(sprite_idx);
                assert(spr->entity_idx == i);

                spr->x = x + spr->offset_x;
                spr->y = y + spr->offset_y;

                sprite_idx = spr->next_in_entity;

                // bounds update
                min_x = spr->x < min_x ? spr->x : min_x;
                min_y = spr->y < min_y ? spr->y : min_y;
                max_x = spr->x + spr->w > max_x ? spr->x + spr->w : max_x;
                max_y = spr->y + spr->h > max_y ? spr->y + spr->h : max_y;
            }

            // update all colliders
            for (u32 collider_idx = ett->collider_begin; collider_idx != 0;) {
                collider *col = collider_get(collider_idx);
                assert(col->entity_idx == i);

                col->x = x + col->offset_x;
                col->y = y + col->offset_y;

                collider_idx = col->next_collider;

                // bounds update
                min_x = col->x < min_x ? col->x : min_x;
                min_y = col->y < min_y ? col->y : min_y;
                max_x = col->x + col->w > max_x ? col->x + col->w : max_x;
                max_y = col->y + col->h > max_y ? col->y + col->h : max_y;
            }

            // update bounds
            ett->bounds_x = min_x;
            ett->bounds_y = min_y;
            ett->bounds_w = max_x - min_x;
            ett->bounds_h = max_y - min_y;

            // update chunk
            {
                u16 chunk_x, chunk_y;
                entity_cal_chunk_pos(x, y, &chunk_x, &chunk_y);

                if (ett->chunk_x != chunk_x || ett->chunk_y != chunk_y) {
                    entity_chunk_remv(i);
                    entity_chunk_add(i);
                }
            }

            // de-flag
            ett->logic_flag &= ~(1 << _ENTITY_LOGIC_FLAG_CREATED);
            ett->logic_flag &= ~(1 << _ENTITY_LOGIC_FLAG_MOVED);
        }
        SET_CLOCK(CLOCK_END_ENTITY_COMPS_UPDATE);

        // FIXME this should be in frame update
        // // culling sprite draw
        // {
        //     f32 camera_bounds_x = camera_x - screen_width * camera_zoom / 2;
        //     f32 camera_bounds_y = camera_y - screen_height * camera_zoom / 2;
        //     f32 camera_bounds_w = screen_width * camera_zoom;
        //     f32 camera_bounds_h = screen_height * camera_zoom;
        //
        //     u32 *ett_in_arr; usize ett_in_arr_len;
        //     entity_query(camera_bounds_x, camera_bounds_y, camera_bounds_w, camera_bounds_h,
        //                  &ett_in_arr, &ett_in_arr_len);
        //
        //     for (usize i = 0; i < ett_in_arr_len; ++i) {
        //         u32 ett_idx = ett_in_arr[i];
        //         assert(ett_idx > 0 && ett_idx < entity_sys.entity_max_idx);
        //         entity *ett = &entity_sys.entity_pool[ett_idx];
        //
        //         // show sprites
        //         for (u32 sprite_idx = ett->sprite_begin; sprite_idx != 0;) {
        //             sprite *spr = sprite_get(sprite_idx);
        //             assert(spr->entity_idx == ett_idx);
        //
        //             spr->show = true;
        //             log_info("%u: %u", time_ms, sprite_idx);
        //
        //             sprite_idx = spr->next_sprite;
        //         }
        //     }
        // }
    }
}

// PRIVATE
// =============================================================================
void entity_cal_chunk_pos(f32 x, f32 y, u16 *chunk_x, u16 *chunk_y) {
    if (chunk_x != nullptr) *chunk_x = (i32)floorf(x / ENTITY_CHUNK_SIZE) + (ENTITY_CHUNK_COUNT / 2 - 1);
    if (chunk_y != nullptr) *chunk_y = (i32)floorf(y / ENTITY_CHUNK_SIZE) + (ENTITY_CHUNK_COUNT / 2 - 1);
}

void entity_chunk_add(u32 ett_idx) {
    assert(ett_idx > 0 && ett_idx < entity_sys.entity_max_idx);

    entity *ett = &entity_sys.entity_pool[ett_idx];

    f32 x, y;
    entity_pos_get(ett_idx, &x, &y, nullptr, nullptr);

    u16 chunk_x, chunk_y;
    entity_cal_chunk_pos(x, y, &chunk_x, &chunk_y);

    u32 *chunk_begin = &entity_sys.chunk_begin_arr[chunk_x + chunk_y * ENTITY_CHUNK_COUNT];

    // linking
    ett->next_in_chunk = *chunk_begin;
    ett->pre_in_chunk = 0;
    if (*chunk_begin != 0) {
        assert(*chunk_begin < entity_sys.entity_max_idx);
        entity_sys.entity_pool[*chunk_begin].pre_in_chunk = ett_idx;
    }
    *chunk_begin = ett_idx;

    // update entity
    ett->chunk_x = chunk_x;
    ett->chunk_y = chunk_y;
}

void entity_chunk_remv(u32 ett_idx) {
    assert(ett_idx > 0 && ett_idx < entity_sys.entity_max_idx);

    entity *ett = &entity_sys.entity_pool[ett_idx];

    u32 *chunk_begin = &entity_sys.chunk_begin_arr[ett->chunk_x + ett->chunk_y * ENTITY_CHUNK_COUNT];

    // linking
    if (*chunk_begin == ett_idx) {
        *chunk_begin = ett->next_in_chunk;
    }
    if (ett->pre_in_chunk != 0) {
        assert(ett->pre_in_chunk < entity_sys.entity_max_idx);
        entity_sys.entity_pool[ett->pre_in_chunk].next_in_chunk = ett->next_in_chunk;
    }
    if (ett->next_in_chunk != 0) {
        assert(ett->next_in_chunk < entity_sys.entity_max_idx);
        entity_sys.entity_pool[ett->next_in_chunk].pre_in_chunk = ett->pre_in_chunk;
    }

    // update entity
    ett->next_in_chunk = ett->pre_in_chunk = 0;
}
