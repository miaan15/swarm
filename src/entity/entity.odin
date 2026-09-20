package entity

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

    chunk_key: u32,

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
entity_create :: proc(pos: [2]f32 = {0, 0}, scale: [2]f32 = {0, 0}, z: i8 = 0) -> (_key: u32, _ptr: ^entity) {
    if entity_sys.entity_pool.len >= entity_sys.entity_pool.cap {
        core.log_error("entity_create: too many entities (%d) => stub", entity_sys.entity_pool.len)
        return 0, &entity_sys.entity_pool.data_list[0]
    }

    key, ptr := pool_create(&entity_sys.entity_pool)

    ptr.pos = pos
    ptr.scale = scale
    ptr.z = z

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

    pool_destroy(&entity_sys.entity_pool, key)

    core.log_debug("destroyed entity [%d]", key)
}

entity_get :: proc(key: u32) -> ^entity {
    if !pool_alive(&entity_sys.entity_pool, key) {
        core.log_error("entity_destroy: entity [%d] invalid (dead or worse) => stub", key)
        return &entity_sys.entity_pool.data_list[0]
    }

    return pool_get(&entity_sys.entity_pool, key)
}

entity_alive :: proc(key: u32) -> bool {
    return pool_alive(&entity_sys.entity_pool, key)
}

// ================================================================================================
entity_sys_update :: proc() {
    _idx: u32 = 1
    for key, ptr in pool_iterate(&entity_sys.entity_pool, &_idx) {
        ptr.pos += ptr.vel

        if ptr.pos != ptr.last_pos {
            ptr.last_pos = ptr.pos

            chunk_mng_update(&entity_sys.entity_chunk, ptr.chunk_key, ptr.pos)
        }
    }
}

