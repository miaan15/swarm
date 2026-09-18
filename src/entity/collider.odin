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
        core.log_error("collider_create: too many colliders (%d) => stub", collider_sys.len)
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

    core.log_debug("created collider [%d]: rect = (%.1f %.1f %.1f %.1f); tag = %d", key, rect[0], rect[1], rect[2], rect[3], tag)

    return key, ptr
}

collider_destroy :: proc(key: u32) {
    if key == 0 || key >= collider_sys.max_key {
        core.log_error("collider_destroy: collider [%d] invalid", key)
        return
    }
    if collider_sys.slot_pool[key] < 0 {
        core.log_warn("collider_destroy: collider [%d] already dead", key)
        return
    }
    idx := u32(-collider_sys.slot_pool[key])

    collider_sys.slot_pool[key] = i32(collider_sys.head)
    collider_sys.head = key

    assert(idx < collider_sys.len)
    if (idx != collider_sys.len - 1) {
        del_ptr := &collider_sys.list[idx]
        repl_ptr := &collider_sys.list[collider_sys.len - 1]
        repl_key := repl_ptr.idx
        assert(repl_key < collider_sys.max_key)

        mem.copy(del_ptr, repl_ptr, size_of(collider))
        del_ptr.idx = repl_key
        collider_sys.slot_pool[repl_key] = -i32(idx)
    }
    collider_sys.len -= 1

    core.log_debug("destroyed collider [%d]", key)
}

collider_get :: proc(key: u32) -> ^collider {
    if key == 0 || key >= collider_sys.max_key {
        core.log_error("collider_get: collider [%d] invalid => stub", key)
        return &collider_sys.list[0]
    }
    if collider_sys.slot_pool[key] >= 0 {
        core.log_warn("collider_get: collider [%d] is dead => stub", key)
        return &collider_sys.list[0]
    }

    idx := -collider_sys.slot_pool[key]
    assert(idx > 0 && idx < i32(collider_sys.len))
    return &collider_sys.list[idx]
}

collider_alive :: proc(key: u32) -> bool {
    if key == 0 || key >= collider_sys.max_key { return false }

    assert(collider_sys.slot_pool[key] > -i32(collider_sys.len))
    return collider_sys.slot_pool[key] < 0
}

// ================================================================================================
collider_sys_update :: proc() {
}
