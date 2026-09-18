package entity

import "../engine/core"

collider :: struct {
    key: u32,

    rect: [4]f32,
    tag: u32,

    ett_owner: u32,
    ett_links: [2]u32,

    ett_offset: [2]f32,
    ett_size: [2]f32,
}

collider_sys : struct {
    collider_pool: pool(collider)
} = {}

// ================================================================================================
collider_sys_init :: proc(cap: u32) {
    pool_init(&collider_sys.collider_pool, cap)
}

// ================================================================================================
collider_create :: proc(rect: [4]f32 = {0, 0, 0, 0}, tag: u32 = 0) -> (_key: u32, _ptr: ^collider) {
    if collider_sys.collider_pool.len >= collider_sys.collider_pool.cap {
        core.log_error("collider_create: too many collider (%d) => stub", collider_sys.collider_pool.len)
        return 0, &collider_sys.collider_pool.data_list[0]
    }

    key, ptr := pool_create(&collider_sys.collider_pool)

    ptr.rect = rect
    ptr.tag = tag

    core.log_debug("created collider [%d]: rect = (%.1f %.1f %.1f %.1f); tag = %d", key, rect[0], rect[1], rect[2], rect[3], tag)

    return key, ptr
}

collider_destroy :: proc(key: u32) {
    if !pool_alive(&collider_sys.collider_pool, key) {
        core.log_error("collider_destroy: collider [%d] invalid (dead or worse)", key)
        return
    }

    pool_destroy(&collider_sys.collider_pool, key)

    core.log_debug("destroyed collider [%d]", key)
}

collider_get :: proc(key: u32) -> ^collider {
    if !pool_alive(&collider_sys.collider_pool, key) {
        core.log_error("collider_destroy: collider [%d] invalid (dead or worse) => stub", key)
        return &collider_sys.collider_pool.data_list[0]
    }

    return pool_get(&collider_sys.collider_pool, key)
}

collider_alive :: proc(key: u32) -> bool {
    return pool_alive(&collider_sys.collider_pool, key)
}

// ================================================================================================
collider_sys_update :: proc() {
}
