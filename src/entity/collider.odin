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
    pool: pool(collider)
} = {}

// ================================================================================================
collider_sys_init :: proc(cap: u32) {
    pool_init(&collider_sys.pool, cap)
}

// ================================================================================================
collider_create :: proc(rect: [4]f32 = {0, 0, 0, 0}, tag: u32 = 0) -> (_key: u32, _ptr: ^collider) {
    if collider_sys.pool.len >= collider_sys.pool.cap {
        core.log_error("collider_create: too many collider (%d) => stub", collider_sys.pool.len)
        return 0, &collider_sys.pool.data_list[0]
    }

    key, ptr := pool_create(&collider_sys.pool)

    ptr.rect = rect
    ptr.tag = tag

    core.log_debug("created collider [%d]: rect = (%.1f %.1f %.1f %.1f); tag = %d", key, rect[0], rect[1], rect[2], rect[3], tag)

    return key, ptr
}

collider_destroy :: proc(key: u32) {
    if key == 0 || key >= collider_sys.pool.max_key {
        core.log_error("collider_destroy: collider [%d] invalid", key)
        return
    }
    if collider_sys.pool.slot_pool[key] >= 0 {
        core.log_warn("collider_destroy: collider [%d] already dead", key)
        return
    }

    pool_destroy(&collider_sys.pool, key)

    core.log_debug("destroyed collider [%d]", key)
}

collider_get :: proc(key: u32) -> ^collider {
    if key == 0 || key >= collider_sys.pool.max_key {
        core.log_error("collider_get: collider [%d] invalid => stub", key)
        return &collider_sys.pool.data_list[0]
    }
    if collider_sys.pool.slot_pool[key] >= 0 {
        core.log_warn("collider_get: collider [%d] is dead => stub", key)
        return &collider_sys.pool.data_list[0]
    }

    return pool_get(&collider_sys.pool, key)
}

collider_alive :: proc(key: u32) -> bool {
    return pool_alive(&collider_sys.pool, key)
}

// ================================================================================================
collider_sys_update :: proc() {
}
