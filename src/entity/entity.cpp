// entity, entity or game object or stuff idk
// use all other stuff in this folder

module;

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstring>

export module entity;

import def;
import mem;
import log;

import pool;
import context;

export import :action;
export import :chunk;
export import :sprite;
export import :collider;
export import :status;

export namespace sw {

constexpr f32 ENTITY_MAX_BOUNDS_SIZE = 256;
constexpr f32 ENTITY_CHUNK_SIZE = 1024;

struct entity {
    u32 pool_key;

    u8 logic_flag; // maybe will be used

    f32 pos[2];
    f32 scale[2];
    i8 z;

    f32 vel[2];

    f32 last_pos[2];

    // chunk
    u32 chunk_key;

    // components
    u32 sprite_list_begin;
    u32 sprite_list_len;

    u32 collider_list_begin;
    u32 collider_list_len;

    u32 status_list_begin;
    u32 status_list_len;
};

struct {
    pool<entity> entity_pool;
    chunk_mng entity_chunk_mng;
} entity_sys = {};

// ================================================================================================

void entity_sys_init(u32 cap) {
    pool_init(&entity_sys.entity_pool, cap, offsetof(entity, pool_key));
    chunk_mng_init(&entity_sys.entity_chunk_mng, cap, ENTITY_CHUNK_SIZE, ENTITY_MAX_BOUNDS_SIZE);
}

// ================================================================================================

void entity_create(f32 pos[2], f32 scale[2], i8 z, u32 *out_key, entity **out_ptr) {
    if (entity_sys.entity_pool.data_list_len >= entity_sys.entity_pool.cap) {
        log_err("entity_create: too many entities (%u) => stub", entity_sys.entity_pool.data_list_len);
        if (out_key) { *out_key = 0; }
        if (out_ptr) { *out_ptr = &entity_sys.entity_pool.data_list_ptr[0]; }
        return;
    }

    u32 entity_key = 0;
    entity *entity_ptr = nullptr;
    pool_create(&entity_sys.entity_pool, &entity_key, &entity_ptr);

    if (pos) {
        entity_ptr->pos[0] = pos[0];
        entity_ptr->pos[1] = pos[1];
    } else {
        entity_ptr->pos[0] = 0;
        entity_ptr->pos[1] = 0;
    }

    if (scale) {
        entity_ptr->scale[0] = scale[0];
        entity_ptr->scale[1] = scale[1];
    } else {
        entity_ptr->scale[0] = 1;
        entity_ptr->scale[1] = 1;
    }

    entity_ptr->z = z;

    // init pos for movement tracking (NAN flags = just created)
    entity_ptr->last_pos[0] = NAN;
    entity_ptr->last_pos[1] = NAN;

    // add to chunk
    chunk_mng_create(&entity_sys.entity_chunk_mng, entity_key, entity_ptr->pos, &entity_ptr->chunk_key, nullptr);

    log_debug("created entity [%u]: pos = (%.1f, %.1f); scale = (%.1f, %.1f); z = %d",
              entity_key, entity_ptr->pos[0], entity_ptr->pos[1], entity_ptr->scale[0], entity_ptr->scale[1], z);

    if (out_key) { *out_key = entity_key; }
    if (out_ptr) { *out_ptr = entity_ptr; }
}

void entity_destroy(u32 entity_key) {
    if (!pool_alive(&entity_sys.entity_pool, entity_key)) {
        log_err("entity_destroy: entity [%u] invalid (dead or worse)", entity_key);
        return;
    }

    entity *entity_ptr = pool_get(&entity_sys.entity_pool, entity_key);

    // destroy from chunk
    chunk_mng_destroy(&entity_sys.entity_chunk_mng, entity_ptr->chunk_key);

    // destroy attached sprites
    u32 sprite_key = entity_ptr->sprite_list_begin;
    while (sprite_key != 0) {
        sprite *sprite_ptr = sprite_get(sprite_key);
        assert(sprite_ptr->owner_entity_key == entity_key);
        u32 next_sprite_key = sprite_ptr->links_in_entity_list[1];

        sprite_destroy(sprite_key);
        sprite_key = next_sprite_key;
    }

    // destroy attached colliders
    u32 collider_key = entity_ptr->collider_list_begin;
    while (collider_key != 0) {
        collider *collider_ptr = collider_get(collider_key);
        assert(collider_ptr->owner_entity_key == entity_key);
        u32 next_collider_key = collider_ptr->links_in_entity_list[1];

        collider_destroy(collider_key);
        collider_key = next_collider_key;
    }

    // destroy attached statuses
    u32 status_idx = entity_ptr->status_list_begin;
    while (status_idx != 0) {
        status *status_ptr = status_get(status_idx);
        assert(status_ptr->owner_entity_key == entity_key);
        u32 next_status_idx = status_ptr->links_in_entity_list[1];

        status_destroy(status_idx);
        status_idx = next_status_idx;
    }

    pool_destroy(&entity_sys.entity_pool, entity_key);

    log_debug("destroyed entity [%u]", entity_key);
}

entity *entity_get(u32 entity_key) {
    if (!pool_alive(&entity_sys.entity_pool, entity_key)) {
        log_err("entity_get: entity [%u] invalid (dead or worse) => stub", entity_key);
        return &entity_sys.entity_pool.data_list_ptr[0];
    }
    return pool_get(&entity_sys.entity_pool, entity_key);
}

bool entity_alive(u32 entity_key) {
    return pool_alive(&entity_sys.entity_pool, entity_key);
}

// ================================================================================================

void entity_new_sprite(u32 entity_key, u32 profile_idx, f32 offset[2], i8 z, f32 scale[2], u32 *out_key, sprite **out_ptr) {
    if (!pool_alive(&entity_sys.entity_pool, entity_key)) {
        log_err("entity_new_sprite: entity [%u] invalid (dead or worse)", entity_key);
        return;
    }

    entity *entity_ptr = pool_get(&entity_sys.entity_pool, entity_key);

    u32 sprite_key = 0;
    sprite *sprite_ptr = nullptr;
    sprite_create(profile_idx, nullptr, 0, &sprite_key, &sprite_ptr);

    if (offset) {
        sprite_ptr->offset_property_entity[0] = offset[0];
        sprite_ptr->offset_property_entity[1] = offset[1];
    } else {
        sprite_ptr->offset_property_entity[0] = 0;
        sprite_ptr->offset_property_entity[1] = 0;
    }

    sprite_ptr->z_property_z = z;

    if (scale) {
        sprite_ptr->scale_property_entity[0] = scale[0];
        sprite_ptr->scale_property_entity[1] = scale[1];
    } else {
        sprite_ptr->scale_property_entity[0] = 1;
        sprite_ptr->scale_property_entity[1] = 1;
    }

    // calculate initial dest rect
    f32 final_scale[2] = {
        sprite_ptr->scale_property_entity[0] * entity_ptr->scale[0],
        sprite_ptr->scale_property_entity[1] * entity_ptr->scale[1]
    };
    sprite_ptr->dest_rect[0] = entity_ptr->pos[0] + sprite_ptr->offset_property_entity[0] * final_scale[0];
    sprite_ptr->dest_rect[1] = entity_ptr->pos[1] + sprite_ptr->offset_property_entity[1] * final_scale[1];
    sprite_ptr->dest_rect[2] = sprite_ptr->src_rect[2] * final_scale[0];
    sprite_ptr->dest_rect[3] = sprite_ptr->src_rect[3] * final_scale[1];

    // pack 64-bit sorting key: [entity z (8b)] [y pos (32b)] [sprite relative z (8b)]
    sprite_ptr->sorting = 0;
    sprite_ptr->sorting |= ((u64)((u8)entity_ptr->z ^ 0x80)) << 56;

    u32 y_bits = 0;
    memcpy(&y_bits, &entity_ptr->pos[1], sizeof(u32));
    y_bits ^= ((u32)(-(i32)(y_bits >> 31)) | 0x80000000u);
    sprite_ptr->sorting |= ((u64)y_bits) << 24;

    sprite_ptr->sorting |= ((u64)((u8)sprite_ptr->z_property_z ^ 0x80)) << 16;

    sprite_ptr->owner_entity_key = entity_key;

    // link into entity's sprite list
    sprite_ptr->links_in_entity_list[0] = 0;
    sprite_ptr->links_in_entity_list[1] = entity_ptr->sprite_list_begin;

    if (entity_ptr->sprite_list_begin != 0) {
        sprite_get(entity_ptr->sprite_list_begin)->links_in_entity_list[0] = sprite_key;
    }

    entity_ptr->sprite_list_begin = sprite_key;
    entity_ptr->sprite_list_len++;

    log_debug("entity [%u] added sprite [%u]: offset = (%.1f, %.1f); z = %d; scale = (%.1f, %.1f)",
              entity_key, sprite_key, sprite_ptr->offset_property_entity[0], sprite_ptr->offset_property_entity[1],
              z, sprite_ptr->scale_property_entity[0], sprite_ptr->scale_property_entity[1]);

    if (out_key) { *out_key = sprite_key; }
    if (out_ptr) { *out_ptr = sprite_ptr; }
}

void entity_new_collider(u32 entity_key, f32 size[2], f32 offset[2], u32 tag, u32 *out_key, collider **out_ptr) {
    if (!pool_alive(&entity_sys.entity_pool, entity_key)) {
        log_err("entity_new_collider: entity [%u] invalid (dead or worse)", entity_key);
        return;
    }

    entity *entity_ptr = pool_get(&entity_sys.entity_pool, entity_key);

    f32 offset_pos[2] = {
        offset ? offset[0] : 0,
        offset ? offset[1] : 0
    };

    // calculate initial collider rect
    f32 rect[4] = {
        entity_ptr->pos[0] + offset_pos[0] * entity_ptr->scale[0],
        entity_ptr->pos[1] + offset_pos[1] * entity_ptr->scale[1],
        size[0] * entity_ptr->scale[0],
        size[1] * entity_ptr->scale[1]
    };

    u32 collider_key = 0;
    collider *collider_ptr = nullptr;
    collider_create(rect, tag, &collider_key, &collider_ptr);

    collider_ptr->size_property_entity[0] = size[0];
    collider_ptr->size_property_entity[1] = size[1];
    collider_ptr->offset_property_entity[0] = offset_pos[0];
    collider_ptr->offset_property_entity[1] = offset_pos[1];
    collider_ptr->owner_entity_key = entity_key;

    // link into entity's collider list
    collider_ptr->links_in_entity_list[0] = 0;
    collider_ptr->links_in_entity_list[1] = entity_ptr->collider_list_begin;

    if (entity_ptr->collider_list_begin != 0) {
        collider_get(entity_ptr->collider_list_begin)->links_in_entity_list[0] = collider_key;
    }

    entity_ptr->collider_list_begin = collider_key;
    entity_ptr->collider_list_len++;

    log_debug("entity [%u] added collider [%u]: size = (%.1f, %.1f); offset = (%.1f, %.1f); tag = %u",
              entity_key, collider_key, size[0], size[1], offset_pos[0], offset_pos[1], tag);

    if (out_key) { *out_key = collider_key; }
    if (out_ptr) { *out_ptr = collider_ptr; }
}

void entity_new_status(u32 entity_key, u32 type, u32 *out_idx, status **out_ptr) {
    if (!pool_alive(&entity_sys.entity_pool, entity_key)) {
        log_err("entity_new_status: entity [%u] invalid (dead or worse)", entity_key);
        return;
    }

    entity *entity_ptr = pool_get(&entity_sys.entity_pool, entity_key);

    u32 status_idx = 0;
    status *status_ptr = nullptr;
    status_create(type, &status_idx, &status_ptr);

    status_ptr->owner_entity_key = entity_key;

    // link into entity's status list
    status_ptr->links_in_entity_list[0] = 0;
    status_ptr->links_in_entity_list[1] = entity_ptr->status_list_begin;

    if (entity_ptr->status_list_begin != 0) {
        status_get(entity_ptr->status_list_begin)->links_in_entity_list[0] = status_idx;
    }

    entity_ptr->status_list_begin = status_idx;
    entity_ptr->status_list_len++;

    log_debug("entity [%u] added status [%u]: type = %u", entity_key, status_idx, type);

    if (out_idx) { *out_idx = status_idx; }
    if (out_ptr) { *out_ptr = status_ptr; }
}

// ================================================================================================

void entity_sys_update() {
    u32 iter_idx = 0;
    u32 entity_key = 0;
    entity *entity_ptr = nullptr;

    while (pool_iterate(&entity_sys.entity_pool, &iter_idx, &entity_key, &entity_ptr)) {
        // velocity
        entity_ptr->pos[0] += entity_ptr->vel[0];
        entity_ptr->pos[1] += entity_ptr->vel[1];

        // update attached components if entity moved
        if (entity_ptr->pos[0] != entity_ptr->last_pos[0] || entity_ptr->pos[1] != entity_ptr->last_pos[1]) {
            entity_ptr->last_pos[0] = entity_ptr->pos[0];
            entity_ptr->last_pos[1] = entity_ptr->pos[1];

            // update chunk
            chunk_mng_update(&entity_sys.entity_chunk_mng, entity_key, entity_ptr->pos);

            // update attached sprites
            u32 sprite_key = entity_ptr->sprite_list_begin;
            while (sprite_key != 0) {
                sprite *sprite_ptr = sprite_get(sprite_key);
                assert(sprite_ptr->owner_entity_key == entity_key);

                f32 final_scale[2] = {
                    sprite_ptr->scale_property_entity[0] * entity_ptr->scale[0],
                    sprite_ptr->scale_property_entity[1] * entity_ptr->scale[1]
                };

                sprite_ptr->dest_rect[0] = entity_ptr->pos[0] + sprite_ptr->offset_property_entity[0] * final_scale[0];
                sprite_ptr->dest_rect[1] = entity_ptr->pos[1] + sprite_ptr->offset_property_entity[1] * final_scale[1];
                sprite_ptr->dest_rect[2] = sprite_ptr->src_rect[2] * final_scale[0];
                sprite_ptr->dest_rect[3] = sprite_ptr->src_rect[3] * final_scale[1];

                // pack sorting
                sprite_ptr->sorting = 0;
                sprite_ptr->sorting |= ((u64)((u8)entity_ptr->z ^ 0x80)) << 56;

                u32 y_bits = 0;
                memcpy(&y_bits, &entity_ptr->pos[1], sizeof(u32));
                y_bits ^= ((u32)(-(i32)(y_bits >> 31)) | 0x80000000u);
                sprite_ptr->sorting |= ((u64)y_bits) << 24;

                sprite_ptr->sorting |= ((u64)((u8)sprite_ptr->z_property_z ^ 0x80)) << 16;

                sprite_key = sprite_ptr->links_in_entity_list[1];
            }

            // update attached colliders
            u32 collider_key = entity_ptr->collider_list_begin;
            while (collider_key != 0) {
                collider *collider_ptr = collider_get(collider_key);
                assert(collider_ptr->owner_entity_key == entity_key);

                collider_ptr->rect[0] = entity_ptr->pos[0] + collider_ptr->offset_property_entity[0] * entity_ptr->scale[0];
                collider_ptr->rect[1] = entity_ptr->pos[1] + collider_ptr->offset_property_entity[1] * entity_ptr->scale[1];
                collider_ptr->rect[2] = collider_ptr->size_property_entity[0] * entity_ptr->scale[0];
                collider_ptr->rect[3] = collider_ptr->size_property_entity[1] * entity_ptr->scale[1];

                collider_key = collider_ptr->links_in_entity_list[1];
            }
        }
    }
}

}
