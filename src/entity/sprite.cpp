// sprite drawn on screen, should be straight forward
// - sprite_profile is just a setted data for easier sprite creation, not really matter in actual draw
// - this suppose to be entity's component, but still can be stand alone

module;

#include <cmath>
#include <cstddef>

export module entity:sprite;

import def;
import mem;
import log;

import pool;

import draw;

import context;

import :chunk;

export namespace sw {

constexpr f32 SPRITE_MAX_BOUNDS_SIZE = 256;
constexpr f32 SPRITE_CHUNK_SIZE = 1024;

struct sprite_profile {
    u32 texture_idx;
    u32 src_rect[4];
};

struct sprite {
    u32 pool_key;

    u32 texture_idx;
    f32 src_rect[4];
    f32 dest_rect[4];
    u64 sorting;

    f32 last_tick_pos[2];
    f32 interpolated_pos[2];

    // chunk
    u32 chunk_key;

    // entity
    u32 owner_entity_key;
    u32 links_in_entity_list[2];

    // entity properties
    f32 offset_property_entity[2];
    i8 z_property_z;
    f32 scale_property_entity[2];

    // once there's entity owner, the entity properties will control the sprite data (dest_rect, sorting)
};

struct {
    // profile
    sprite_profile *profile_list_ptr;
    u32 profile_list_cap;
    u32 profile_list_len;

    // sprite
    pool<sprite> sprite_pool;
    chunk_mng sprite_chunk_mng;
} sprite_sys = {};

// ================================================================================================

void sprite_sys_init(u32 profile_cap, u32 sprite_cap) {
    // sprite profile
    sprite_sys.profile_list_ptr = (sprite_profile*)arena_alloc(&omni_arena, profile_cap * sizeof(sprite_profile));
    sprite_sys.profile_list_cap = profile_cap;

    // stub
    sprite_sys.profile_list_len = 1;
    sprite_sys.profile_list_ptr[0] = sprite_profile{ 0, { 0, 0, 32, 32 } };

    // sprite
    pool_init(&sprite_sys.sprite_pool, sprite_cap, offsetof(sprite, pool_key));
    chunk_mng_init(&sprite_sys.sprite_chunk_mng, sprite_cap, SPRITE_CHUNK_SIZE, SPRITE_MAX_BOUNDS_SIZE);
}

// ================================================================================================

u32 sprite_profile_create(u32 texture_idx, u32 src_rect[4]) {
    if (sprite_sys.profile_list_len >= sprite_sys.profile_list_cap) {
        log_err("sprite_profile_create: too much sprite profiles (%u) => stub", sprite_sys.profile_list_len);
        return 0;
    }

    u32 profile_idx = sprite_sys.profile_list_len;
    sprite_sys.profile_list_ptr[profile_idx] = sprite_profile{
        texture_idx,
        { src_rect[0], src_rect[1], src_rect[2], src_rect[3] }
    };
    sprite_sys.profile_list_len++;

    log_debug("created sprite profile [%u]: texture = [%u], rect = (%u, %u, %u, %u)",
              profile_idx, texture_idx, src_rect[0], src_rect[1], src_rect[2], src_rect[3]);

    return profile_idx;
}

sprite_profile sprite_profile_get(u32 profile_idx) {
    if (profile_idx == 0 || profile_idx >= sprite_sys.profile_list_len) {
        log_err("sprite_profile_get: profile [%u] invalid => stub", profile_idx);
        return sprite_sys.profile_list_ptr[0];
    }
    return sprite_sys.profile_list_ptr[profile_idx];
}

// ================================================================================================

void sprite_create(u32 profile_idx, f32 dest_rect[4], u64 sorting, u32 *out_sprite_key, sprite **out_sprite_ptr) {
    // create sprite from profile, update chunk

    if (sprite_sys.sprite_pool.data_list_len >= sprite_sys.sprite_pool.cap) {
        log_err("sprite_create: too many sprite (%u) => stub", sprite_sys.sprite_pool.data_list_len);
        if (out_sprite_key) { *out_sprite_key = 0; }
        if (out_sprite_ptr) { *out_sprite_ptr = &sprite_sys.sprite_pool.data_list_ptr[0]; }
        return;
    }

    u32 sprite_key = 0;
    sprite *sprite_ptr = nullptr;
    pool_create(&sprite_sys.sprite_pool, &sprite_key, &sprite_ptr);

    // set data from profile
    sprite_profile profile = sprite_profile_get(profile_idx);
    sprite_ptr->texture_idx = profile.texture_idx;
    sprite_ptr->src_rect[0] = (f32)profile.src_rect[0];
    sprite_ptr->src_rect[1] = (f32)profile.src_rect[1];
    sprite_ptr->src_rect[2] = (f32)profile.src_rect[2];
    sprite_ptr->src_rect[3] = (f32)profile.src_rect[3];

    // set dest rect
    if (dest_rect) {
        sprite_ptr->dest_rect[0] = dest_rect[0];
        sprite_ptr->dest_rect[1] = dest_rect[1];
        sprite_ptr->dest_rect[2] = dest_rect[2];
        sprite_ptr->dest_rect[3] = dest_rect[3];
    } else {
        sprite_ptr->dest_rect[0] = 0;
        sprite_ptr->dest_rect[1] = 0;
        sprite_ptr->dest_rect[2] = 0;
        sprite_ptr->dest_rect[3] = 0;
    }

    sprite_ptr->sorting = sorting;

    // init those pos for interpolation; NAN is just a way to flag "just created"
    sprite_ptr->last_tick_pos[0] = NAN;
    sprite_ptr->last_tick_pos[1] = NAN;
    sprite_ptr->interpolated_pos[0] = sprite_ptr->dest_rect[0];
    sprite_ptr->interpolated_pos[1] = sprite_ptr->dest_rect[1];

    // register into chunk
    f32 chunk_rect[4] = {
        sprite_ptr->interpolated_pos[0],
        sprite_ptr->interpolated_pos[1],
        sprite_ptr->dest_rect[2],
        sprite_ptr->dest_rect[3]
    };
    f32 center_pos[2];
    chunk_cal_center_rect(chunk_rect, center_pos);

    chunk_mng_create(&sprite_sys.sprite_chunk_mng, sprite_key, center_pos, &sprite_ptr->chunk_key, nullptr);

    log_debug("created sprite [%u]: profile = [%u]; dest = (%.1f, %.1f, %.1f, %.1f); sort = %llu",
              sprite_key, profile_idx, sprite_ptr->dest_rect[0], sprite_ptr->dest_rect[1],
              sprite_ptr->dest_rect[2], sprite_ptr->dest_rect[3], (unsigned long long)sorting);

    if (out_sprite_key) { *out_sprite_key = sprite_key; }
    if (out_sprite_ptr) { *out_sprite_ptr = sprite_ptr; }
}

void sprite_destroy(u32 sprite_key) {
    if (!pool_alive(&sprite_sys.sprite_pool, sprite_key)) {
        log_err("sprite_destroy: sprite [%u] invalid (dead or worse)", sprite_key);
        return;
    }

    sprite *sprite_ptr = pool_get(&sprite_sys.sprite_pool, sprite_key);

    // destroy from stuff
    chunk_mng_destroy(&sprite_sys.sprite_chunk_mng, sprite_ptr->chunk_key);
    pool_destroy(&sprite_sys.sprite_pool, sprite_key);

    log_debug("destroyed sprite [%u]", sprite_key);
}

sprite *sprite_get(u32 sprite_key) {
    if (!pool_alive(&sprite_sys.sprite_pool, sprite_key)) {
        log_trace("sprite_get: sprite [%u] invalid (dead or worse) => stub", sprite_key);
        return &sprite_sys.sprite_pool.data_list_ptr[0];
    }
    return pool_get(&sprite_sys.sprite_pool, sprite_key);
}

bool sprite_alive(u32 sprite_key) {
    return pool_alive(&sprite_sys.sprite_pool, sprite_key);
}

// ================================================================================================

void sprite_sys_update_early() {
    // cache current tick positions for interpolation

    u32 iter_idx = 0;
    u32 sprite_key = 0;
    sprite *sprite_ptr = nullptr;

    while (pool_iterate(&sprite_sys.sprite_pool, &iter_idx, &sprite_key, &sprite_ptr)) {
        if (!std::isnan(sprite_ptr->last_tick_pos[0]) && !std::isnan(sprite_ptr->last_tick_pos[1])) {
            sprite_ptr->last_tick_pos[0] = sprite_ptr->dest_rect[0];
            sprite_ptr->last_tick_pos[1] = sprite_ptr->dest_rect[1];
        }
    }
}

void sprite_sys_draw() {
    u32 iter_idx = 0;
    u32 sprite_key = 0;
    sprite *sprite_ptr = nullptr;

    while (pool_iterate(&sprite_sys.sprite_pool, &iter_idx, &sprite_key, &sprite_ptr)) {
        f32 current_pos[2] = { sprite_ptr->dest_rect[0], sprite_ptr->dest_rect[1] };

        // interpolate if previous tick position is valid, otherwise snap directly (just created)
        if (!std::isnan(sprite_ptr->last_tick_pos[0]) && !std::isnan(sprite_ptr->last_tick_pos[1])) {
            sprite_ptr->interpolated_pos[0] = sprite_ptr->last_tick_pos[0] + (current_pos[0] - sprite_ptr->last_tick_pos[0]) * (f32)tick_frame_alpha;
            sprite_ptr->interpolated_pos[1] = sprite_ptr->last_tick_pos[1] + (current_pos[1] - sprite_ptr->last_tick_pos[1]) * (f32)tick_frame_alpha;
        } else {
            sprite_ptr->interpolated_pos[0] = current_pos[0];
            sprite_ptr->interpolated_pos[1] = current_pos[1];
            sprite_ptr->last_tick_pos[0] = current_pos[0];
            sprite_ptr->last_tick_pos[1] = current_pos[1];
        }

        // update chunk
        f32 chunk_rect[4] = {
            sprite_ptr->interpolated_pos[0],
            sprite_ptr->interpolated_pos[1],
            sprite_ptr->dest_rect[2],
            sprite_ptr->dest_rect[3]
        };
        f32 center_pos[2];
        chunk_cal_center_rect(chunk_rect, center_pos);
        chunk_mng_update(&sprite_sys.sprite_chunk_mng, sprite_key, center_pos);

        //draw
        draw_call *drw = draw_call_make();
        drw->type = draw_type::TEXTURE;
        drw->texture.texture_idx = sprite_ptr->texture_idx;
        drw->texture.src_rect[0] = sprite_ptr->src_rect[0];
        drw->texture.src_rect[1] = sprite_ptr->src_rect[1];
        drw->texture.src_rect[2] = sprite_ptr->src_rect[2];
        drw->texture.src_rect[3] = sprite_ptr->src_rect[3];
        drw->texture.dest_rect[0] = sprite_ptr->interpolated_pos[0];
        drw->texture.dest_rect[1] = sprite_ptr->interpolated_pos[1];
        drw->texture.dest_rect[2] = sprite_ptr->dest_rect[2];
        drw->texture.dest_rect[3] = sprite_ptr->dest_rect[3];
        drw->sorting = sprite_ptr->sorting;
    }
}

}
