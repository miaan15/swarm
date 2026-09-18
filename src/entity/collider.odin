package entity

import "core:mem"
import "../engine/core"
import "../global"

collider :: struct {
    idx: u32,

    rect: [4]f32,
    tag: u32,

    ett_owner: u32,
    ett_links: [2]u32,

    ett_offset: [2]f32,
    ett_size: [2]f32,
}

collider_sys : struct {
    slot_pool: [^]i32,
    list: [^]collider,
    cap, head, max_key, len: u32,
} = {}

// ================================================================================================
collider_sys_init :: proc(cap: u32) {
    collider_sys.slot_pool = transmute([^]i32)core.arena_alloc(&global.omni_arena, cap * size_of(i32))
    collider_sys.list = transmute([^]collider)core.arena_alloc(&global.omni_arena, cap * size_of(collider))
    collider_sys.cap = cap

    // stub
    collider_sys.head = 1
    collider_sys.max_key = 1
    collider_sys.len = 1
}

// ================================================================================================
collider_create :: proc(rect: [4]f32 = {0, 0, 0, 0}, tag: u32 = 0) -> (_key: u32, _ptr: ^collider) {
    if collider_sys.len >= collider_sys.cap {
        core.log_error("collider_create: too many colliders (%u) => stub", collider_sys.len)
        return 0, &collider_sys.list[0]
    }

    key := collider_sys.head
    if key == collider_sys.max_key {
        collider_sys.max_key += 1
        collider_sys.head += 1
    } else {
        collider_sys.head = u32(collider_sys.slot_pool[key])
    }
    collider_sys.slot_pool[key] = -i32(collider_sys.len)

    assert(collider_sys.len < collider_sys.cap)
    ptr := &collider_sys.list[collider_sys.len]
    collider_sys.len += 1

    mem.zero(ptr, size_of(collider))
    ptr.idx = key

    ptr.rect = rect
    ptr.tag = tag

    core.log_debug("created collider [%u]: rect = (%.1f %.1f %.1f %.1f); tag = %u", key, rect[0], rect[1], rect[2], rect[3], tag)

    return key, ptr
}

collider_destroy :: proc(key: u32) {
    if collider_sys.slot_pool[key] < 0 {
        core.log_warn("collider_destroy: collider [%u] already dead", key)
        return
    }
    idx := u32(-collider_sys.slot_pool[key])

    collider_sys.slot_pool[key] = i32(collider_sys.head)
    collider_sys.head = key
    collider_sys.len -= 1

    assert(idx > 0 && idx < collider_sys.len)
    ptr := &collider_sys.list[idx]

    core.log_debug("destroyed collider [%u]", key)

    mem.zero(ptr, size_of(collider))
}

collider_get :: proc(key: u32) -> ^collider {
    if key == 0 || key >= collider_sys.max_key {
        core.log_error("collider_get: collider [%u] invalid => stub", key)
        return &collider_sys.list[0]
    }

    idx := u32(-collider_sys.slot_pool[key])
    if idx <= 0 {
        core.log_error("collider_get: collider [%u] is dead => stub", key)
        return &collider_sys.list[0]
    }

    assert(idx < collider_sys.len)
    return &collider_sys.list[idx]
}

collider_alive :: proc(key: u32) -> bool {
    if key == 0 || key >= collider_sys.max_key { return false }

    idx := u32(-collider_sys.slot_pool[key])

    assert(idx < collider_sys.len)
    return idx > 0
}

// ================================================================================================
collider_sys_update :: proc() {
}
