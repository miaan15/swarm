package entity

import "../engine/core"

entity :: struct {
    key: u32,

    logic_flag: u8,

    pos: [2]f32,
    scale: [2]f32,
    z: i8,

    vel: [2]f32,

    spr_begin, spr_len: u32,
    col_begin, col_len: u32,

    status_begin, status_len: u32,
}

entity_sys : struct {
    entity_pool: pool(entity),
    chunk_mng: chunk_mng
} = {}

// ================================================================================================
entity_sys_init :: proc(cap: u32) {
    pool_init(&entity_sys.entity_pool, cap)
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

    core.log_debug("created entity [%d]: pos = (%.1f %.1f); scale = (%.1f %.1f); z = %d", key, pos[0], pos[1], scale[0], scale[1], z)

    return key, ptr
}

entity_destroy :: proc(key: u32) {
    if !pool_alive(&entity_sys.entity_pool, key) {
        core.log_error("entity_destroy: entity [%d] invalid (dead or worse)", key)
        return
    }

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

}

