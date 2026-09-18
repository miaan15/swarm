package entity

import "core:fmt"
import "core:strings"
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
        core.log_error("entity_create: too many entities (%d) => stub", entity_sys.len)
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

    core.log_debug("created entity [%d]: pos = (%.1f %.1f); scale = (%.1f %.1f); z = %d", key, pos[0], pos[1], scale[0], scale[1], z)

    return key, ptr
}

entity_destroy :: proc(key: u32) {
    if key == 0 || key >= entity_sys.max_key {
        core.log_error("entity_destroy: entity [%d] invalid", key)
        return
    }
    if entity_sys.slot_pool[key] >= 0 {
        core.log_warn("entity_destroy: entity [%d] already dead", key)
        return
    }
    idx := u32(-entity_sys.slot_pool[key])

    entity_sys.slot_pool[key] = i32(entity_sys.head)
    entity_sys.head = key

    assert(idx < entity_sys.len)
    if (idx != entity_sys.len - 1) {
        del_ptr := &entity_sys.list[idx]
        repl_ptr := &entity_sys.list[entity_sys.len - 1]
        repl_key := repl_ptr.idx
        assert(repl_key < entity_sys.max_key)

        mem.copy(del_ptr, repl_ptr, size_of(entity))
        del_ptr.idx = repl_key
        entity_sys.slot_pool[repl_key] = -i32(idx)
    }
    entity_sys.len -= 1

    core.log_debug("destroyed entity [%d]", key)
}

entity_get :: proc(key: u32) -> ^entity {
    if key == 0 || key >= entity_sys.max_key {
        core.log_error("entity_get: entity [%d] invalid => stub", key)
        return &entity_sys.list[0]
    }
    if entity_sys.slot_pool[key] >= 0 {
        core.log_warn("entity_get: entity [%d] is dead => stub", key)
        return &entity_sys.list[0]
    }

    idx := -entity_sys.slot_pool[key]
    assert(idx > 0 && idx < i32(entity_sys.len))
    return &entity_sys.list[idx]
}

entity_alive :: proc(key: u32) -> bool {
    if key == 0 || key >= entity_sys.max_key { return false }

    assert(entity_sys.slot_pool[key] > -i32(entity_sys.len))
    return entity_sys.slot_pool[key] < 0
}

// ================================================================================================
entity_sys_update :: proc() {

}

// DEBUG TEST
// ================================================================================================
_entity_sys_validate :: proc() -> bool {
    cnt_len: u32 = 0
    max: u32 = 0
    for key in 1..<entity_sys.max_key {
        if !entity_alive(key) { continue; }
        cnt_len += 1
        idx := u32(-entity_sys.slot_pool[key])
        if idx == 0 || idx >= entity_sys.len {
            core.log_trace("entity [%d] should not slot = %d", key, entity_sys.slot_pool[key])
            return false
        }
        if entity_sys.list[idx].idx != key {
            core.log_trace("entity [%d] idx = %d", key, entity_sys.list[idx].idx, entity_sys.slot_pool[key])
            return false
        }
        max = idx if idx > max else max
    }
    if cnt_len != entity_sys.len - 1 {
        core.log_trace("entity len = %d but actually %d", cnt_len, entity_sys.len)
        return false
    }
    if max != entity_sys.len - 1 {
        core.log_trace("entity max = %d but only %d", max, entity_sys.len)
        return false
    }
    return true
}

_entity_sys_debug_log :: proc() {
    b := strings.builder_make(context.temp_allocator)

    //
    fmt.sbprintf(&b, "%s\n", "VALID" if _entity_sys_validate() else "INVALID*")

    //
    for key in 0 ..< entity_sys.max_key {
        fmt.sbprintf(&b, " %2d  ", key)
    }
    strings.write_byte(&b, '\n')

    //
    for key in 0 ..< entity_sys.max_key {
        if entity_alive(key) {
            val := u32(-entity_sys.slot_pool[key])
            fmt.sbprintf(&b, "[%2d] ", val)
        } else {
            strings.write_string(&b, "[  ] ")
        }
    }
    strings.write_byte(&b, '\n')

    for key in 0 ..< entity_sys.max_key {
        if !entity_alive(key) {
            val := u32(entity_sys.slot_pool[key])
            fmt.sbprintf(&b, "[%2d] ", val)
        } else {
            strings.write_string(&b, "[  ] ")
        }
    }
    strings.write_byte(&b, '\n')

    core.log_info("%s", strings.to_string(b))
}
