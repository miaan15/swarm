package entity

import "core:mem"
import "../engine/core"
import "../global"

entity :: struct {
    idx: u32,

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
    slot_pool: [^]i32,
    list: [^]entity,
    cap, head, max_key, len: u32,
} = {}

// ================================================================================================
entity_sys_init :: proc(cap: u32) {
    entity_sys.slot_pool = transmute([^]i32)core.arena_alloc(&global.omni_arena, cap * size_of(i32))
    entity_sys.list = transmute([^]entity)core.arena_alloc(&global.omni_arena, cap * size_of(entity))
    entity_sys.cap = cap

    // stub
    entity_sys.head = 1
    entity_sys.max_key = 1
    entity_sys.len = 1
}

// ================================================================================================
entity_create :: proc(pos: [2]f32 = {0, 0}, scale: [2]f32 = {0, 0}, z: i8 = 0) -> (_key: u32, _ptr: ^entity) {
    if entity_sys.len >= entity_sys.cap {
        core.log_error("entity_create: too many entities (%u) => stub", entity_sys.len)
        return 0, &entity_sys.list[0]
    }

    key := entity_sys.head
    if key == entity_sys.max_key {
        entity_sys.max_key += 1
        entity_sys.head += 1
    } else {
        entity_sys.head = u32(entity_sys.slot_pool[key])
    }
    entity_sys.slot_pool[key] = -i32(entity_sys.len)

    assert(entity_sys.len < entity_sys.cap)
    ptr := &entity_sys.list[entity_sys.len]
    entity_sys.len += 1

    mem.zero(ptr, size_of(entity))
    ptr.idx = key

    ptr.pos = pos
    ptr.scale = scale
    ptr.z = z

    core.log_debug("created entity [%u]: pos = (%.1f %.1f); scale = (%.1f %.1f); z = %d", key, pos[0], pos[1], scale[0], scale[1], z)

    return key, ptr
}

entity_destroy :: proc(key: u32) {
    if entity_sys.slot_pool[key] < 0 {
        core.log_warn("entity_destroy: entity [%u] already dead", key)
        return
    }
    idx := u32(-entity_sys.slot_pool[key])

    entity_sys.slot_pool[key] = i32(entity_sys.head)
    entity_sys.head = key
    entity_sys.len -= 1

    assert(idx > 0 && idx < entity_sys.len)
    ptr := &entity_sys.list[idx]

    core.log_debug("destroyed entity [%u]", key)

    mem.zero(ptr, size_of(entity))
}

entity_get :: proc(key: u32) -> ^entity {
    if key == 0 || key >= entity_sys.max_key {
        core.log_error("entity_get: entity [%u] invalid => stub", key)
        return &entity_sys.list[0]
    }

    idx := u32(-entity_sys.slot_pool[key])
    if idx <= 0 {
        core.log_error("entity_get: entity [%u] is dead => stub", key)
        return &entity_sys.list[0]
    }

    assert(idx < entity_sys.len)
    return &entity_sys.list[idx]
}

entity_alive :: proc(key: u32) -> bool {
    if key == 0 || key >= entity_sys.max_key { return false }

    idx := u32(-entity_sys.slot_pool[key])

    assert(idx < entity_sys.len)
    return idx > 0
}

// ================================================================================================
entity_sys_update :: proc() {

}
