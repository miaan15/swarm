package entity

import "core:math"
import "../engine/core"

ENTITY_MAX_BOUNDS_SIZE: f32 : 512

entity :: struct {
    key: u32,

    logic_flag: u8,

    pos: [2]f32,
    scale: [2]f32,
    z: i8,

    vel: [2]f32,

    last_pos: [2]f32,

    // chunk
    chunk_key: u32,

    // components
    spr_begin, spr_len: u32,
    col_begin, col_len: u32,

    status_begin, status_len: u32,
}

entity_sys : struct {
    entity_pool: pool(entity),
    entity_chunk: chunk_mng
} = {}

// ================================================================================================
entity_sys_init :: proc(cap: u32) {
    pool_init(&entity_sys.entity_pool, cap)
    chunk_mng_init(&entity_sys.entity_chunk, 1024, cap)
}

// ================================================================================================
entity_create :: proc(pos: [2]f32 = {0, 0}, scale: [2]f32 = {1, 1}, z: i8 = 0) -> (_key: u32, _ptr: ^entity) {
    if entity_sys.entity_pool.len >= entity_sys.entity_pool.cap {
        core.log_error("entity_create: too many entities (%d) => stub", entity_sys.entity_pool.len)
        return 0, &entity_sys.entity_pool.data_list[0]
    }

    key, ptr := pool_create(&entity_sys.entity_pool)

    ptr.pos = pos
    ptr.scale = scale
    ptr.z = z

    ptr.last_pos = { math.nan_f32(), math.nan_f32() }

    ptr.chunk_key, _ = chunk_mng_create(&entity_sys.entity_chunk, key, pos)

    core.log_debug("created entity [%d]: pos = (%.1f, %.1f); scale = (%.1f, %.1f); z = %d", key, pos[0], pos[1], scale[0], scale[1], z)

    return key, ptr
}

entity_destroy :: proc(key: u32) {
    if !pool_alive(&entity_sys.entity_pool, key) {
        core.log_error("entity_destroy: entity [%d] invalid (dead or worse)", key)
        return
    }

    ptr := pool_get(&entity_sys.entity_pool, key)

    chunk_mng_destroy(&entity_sys.entity_chunk, ptr.chunk_key)

    // destroy sprite
    for spr_key := ptr.spr_begin; spr_key != 0; {
        spr := sprite_get(spr_key)
        assert(spr.ett_owner == key)

        sprite_destroy(spr_key)

        spr_key = spr.ett_links[1]
    }

    // destroy collider
    for col_key := ptr.col_begin; col_key != 0; {
        col := collider_get(col_key)
        assert(col.ett_owner == key)

        collider_destroy(col_key)

        col_key = col.ett_links[1]
    }

    // destroy status
    for status_key := ptr.status_begin; status_key != 0; {
        status := status_get(status_key)
        assert(status.ett_owner == key)

        status_destroy(status_key)

        status_key = status.ett_links[1]
    }

    pool_destroy(&entity_sys.entity_pool, key)

    core.log_debug("destroyed entity [%d]", key)
}

entity_get :: proc(key: u32) -> ^entity {
    if !pool_alive(&entity_sys.entity_pool, key) {
        core.log_error("entity_get: entity [%d] invalid (dead or worse) => stub", key)
        return &entity_sys.entity_pool.data_list[0]
    }

    return pool_get(&entity_sys.entity_pool, key)
}

entity_alive :: proc(key: u32) -> bool {
    return pool_alive(&entity_sys.entity_pool, key)
}

// ================================================================================================
entity_new_sprite :: proc(key: u32, profile_idx: u32, offset: [2]f32 = {0, 0}, z: i8 = 0, scale: [2]f32 = {1, 1}) -> (_key: u32, _ptr: ^sprite) {
    if !pool_alive(&entity_sys.entity_pool, key) {
        core.log_error("entity_create_sprite: entity [%d] invalid (dead or worse)", key)
        return
    }

    ptr := pool_get(&entity_sys.entity_pool, key)

    spr_key, spr := sprite_create(profile_idx)
    spr.ett_offset = offset
    spr.ett_z = z
    spr.ett_scale = scale

    {
        scale := spr.ett_scale * ptr.scale
        spr.dest[0] = ptr.pos[0] + spr.ett_offset[0] * scale[0]
        spr.dest[1] = ptr.pos[1] + spr.ett_offset[1] * scale[1]
        spr.dest[2] = spr.src[2] * scale[0]
        spr.dest[3] = spr.src[3] * scale[1]

        spr.sorting = 0
        spr.sorting |= u64(transmute(u8)ptr.z ~ 0x80) << 56

        _y: u32 = transmute(u32)ptr.pos[1]
        _y ~= (u32(-i32(_y >> 31)) | 0x80000000)
        spr.sorting |= u64(_y) << 24

        spr.sorting |= u64(transmute(u8)spr.ett_z ~ 0x80) << 16
    }

    spr.ett_owner = key

    // link
    spr.ett_links[0] = 0
    spr.ett_links[1] = ptr.spr_begin

    if ptr.spr_begin != 0 { sprite_get(ptr.spr_begin).ett_links[0] = spr_key }

    ptr.spr_begin = spr_key

    ptr.spr_len += 1

    core.log_debug("entity [%d] added sprite [%d]: offset = (%.1f, %.1f); ; z = %d; scale = (%.1f, %.1f)", key, spr_key, offset[0], offset[1], z, scale[0], scale[1])

    return spr_key, spr
}

entity_new_collider :: proc(key: u32, size: [2]f32, offset: [2]f32 = {0, 0}, tag: u32 = 0) -> (_key: u32, _ptr: ^collider){
    if !pool_alive(&entity_sys.entity_pool, key) {
        core.log_error("entity_create_collider: entity [%d] invalid (dead or worse)", key)
        return
    }

    ptr := pool_get(&entity_sys.entity_pool, key)

    col_key, col := collider_create()
    col.ett_size = size
    col.ett_offset = offset
    col.tag = tag

    {
        col.rect[0] = ptr.pos[0] + col.ett_offset[0] * ptr.scale[0]
        col.rect[1] = ptr.pos[1] + col.ett_offset[1] * ptr.scale[1]
        col.rect[2] = col.ett_size[0] * col.ett_offset[0]
        col.rect[3] = col.ett_size[1] * col.ett_offset[1]
    }

    col.ett_owner = key

    // link
    col.ett_links[0] = 0
    col.ett_links[1] = ptr.col_begin

    if ptr.col_begin != 0 { collider_get(ptr.col_begin).ett_links[0] = col_key }

    ptr.col_begin = col_key

    ptr.col_len += 1

    core.log_debug("entity [%d] added collider [%d]: size = (%.1f, %.1f); offset = (%.1f, %.1f); tag = %d", key, col_key, size[0], size[1], offset[0], offset[1], tag)

    return col_key, col
}

entity_new_status :: proc(key: u32, type: u32) -> (_key: u32, _ptr: ^status) {
    if !pool_alive(&entity_sys.entity_pool, key) {
        core.log_error("entity_create_status: entity [%d] invalid (dead or worse)", key)
        return
    }

    ptr := pool_get(&entity_sys.entity_pool, key)

    status_key, status := status_create(type)

    status.ett_owner = key

    // link
    status.ett_links[0] = 0
    status.ett_links[1] = ptr.status_begin

    if ptr.status_begin != 0 { status_get(ptr.status_begin).ett_links[0] = status_key }

    ptr.status_begin = status_key

    ptr.status_len += 1

    core.log_debug("entity [%d] added status [%d]: type = %d", key, status_key, type)

    return status_key, status
}

// ================================================================================================
entity_sys_update :: proc() {
    _idx: u32 = 1
    for key, ptr in pool_iterate(&entity_sys.entity_pool, &_idx) {
        ptr.pos += ptr.vel

        if ptr.pos != ptr.last_pos { // if moved
            ptr.last_pos = ptr.pos

            chunk_mng_update(&entity_sys.entity_chunk, ptr.chunk_key, ptr.pos)

            // sprite
            for spr_key := ptr.spr_begin; spr_key != 0; {
                spr := sprite_get(spr_key)
                assert(spr.ett_owner == key)

                scale := spr.ett_scale * ptr.scale
                spr.dest[0] = ptr.pos[0] + spr.ett_offset[0] * scale[0]
                spr.dest[1] = ptr.pos[1] + spr.ett_offset[1] * scale[1]
                spr.dest[2] = spr.src[2] * scale[0]
                spr.dest[3] = spr.src[3] * scale[1]

                spr.sorting = 0
                spr.sorting |= u64(transmute(u8)ptr.z ~ 0x80) << 56

                _y: u32 = transmute(u32)ptr.pos[1]
                _y ~= (u32(-i32(_y >> 31)) | 0x80000000)
                spr.sorting |= u64(_y) << 24

                spr.sorting |= u64(transmute(u8)spr.ett_z ~ 0x80) << 16

                spr_key = spr.ett_links[1]
            }

            // collider
            for col_key := ptr.col_begin; col_key != 0; {
                col := collider_get(col_key)
                assert(col.ett_owner == key)

                col.rect[0] = ptr.pos[0] + col.ett_offset[0] * ptr.scale[0]
                col.rect[1] = ptr.pos[1] + col.ett_offset[1] * ptr.scale[1]
                col.rect[2] = col.ett_size[0] * col.ett_offset[0]
                col.rect[3] = col.ett_size[1] * col.ett_offset[1]

                col_key = col.ett_links[1]
            }
        }
    }
}

