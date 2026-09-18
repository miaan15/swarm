package entity

import "core:fmt"
import "core:strings"
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
    pool: pool(entity)
} = {}

// ================================================================================================
entity_sys_init :: proc(cap: u32) {
    pool_init(&entity_sys.pool, cap)
}

// ================================================================================================
entity_create :: proc(pos: [2]f32 = {0, 0}, scale: [2]f32 = {0, 0}, z: i8 = 0) -> (_key: u32, _ptr: ^entity) {
    if entity_sys.pool.len >= entity_sys.pool.cap {
        core.log_error("entity_create: too many entities (%d) => stub", entity_sys.pool.len)
        return 0, &entity_sys.pool.data_list[0]
    }

    key, ptr := pool_create(&entity_sys.pool)

    ptr.pos = pos
    ptr.scale = scale
    ptr.z = z

    core.log_debug("created entity [%d]: pos = (%.1f %.1f); scale = (%.1f %.1f); z = %d", key, pos[0], pos[1], scale[0], scale[1], z)

    return key, ptr
}

entity_destroy :: proc(key: u32) {
    if key == 0 || key >= entity_sys.pool.max_key {
        core.log_error("entity_destroy: entity [%d] invalid", key)
        return
    }
    if entity_sys.pool.slot_pool[key] >= 0 {
        core.log_warn("entity_destroy: entity [%d] already dead", key)
        return
    }

    pool_destroy(&entity_sys.pool, key)

    core.log_debug("destroyed entity [%d]", key)
}

entity_get :: proc(key: u32) -> ^entity {
    if key == 0 || key >= entity_sys.pool.max_key {
        core.log_error("entity_get: entity [%d] invalid => stub", key)
        return &entity_sys.pool.data_list[0]
    }
    if entity_sys.pool.slot_pool[key] >= 0 {
        core.log_warn("entity_get: entity [%d] is dead => stub", key)
        return &entity_sys.pool.data_list[0]
    }

    return pool_get(&entity_sys.pool, key)
}

entity_alive :: proc(key: u32) -> bool {
    return pool_alive(&entity_sys.pool, key)
}

// ================================================================================================
entity_sys_update :: proc() {

}

// DEBUG TEST
// ================================================================================================
_entity_sys_validate :: proc() -> bool {
    cnt_len: u32 = 0
    max: u32 = 0
    for key in 1..<entity_sys.pool.max_key {
        if !entity_alive(key) { continue; }
        cnt_len += 1
        idx := u32(-entity_sys.pool.slot_pool[key])
        if idx == 0 || idx >= entity_sys.pool.len {
            core.log_trace("entity [%d] should not slot = %d", key, entity_sys.pool.slot_pool[key])
            return false
        }
        if entity_sys.pool.data_list[idx].key != key {
            core.log_trace("entity [%d] idx = %d", key, entity_sys.pool.data_list[idx].key, entity_sys.pool.slot_pool[key])
            return false
        }
        max = idx if idx > max else max
    }
    if cnt_len != entity_sys.pool.len - 1 {
        core.log_trace("entity len = %d but actually %d", cnt_len, entity_sys.pool.len)
        return false
    }
    if max != entity_sys.pool.len - 1 {
        core.log_trace("entity max = %d but only %d", max, entity_sys.pool.len)
        return false
    }
    return true
}

_entity_sys_debug_log :: proc() {
    b := strings.builder_make(context.temp_allocator)

    //
    fmt.sbprintf(&b, "%s\n", "VALID" if _entity_sys_validate() else "INVALID*")

    //
    for key in 0 ..< entity_sys.pool.max_key {
        fmt.sbprintf(&b, " %2d  ", key)
    }
    strings.write_byte(&b, '\n')

    //
    for key in 0 ..< entity_sys.pool.max_key {
        if entity_alive(key) {
            val := u32(-entity_sys.pool.slot_pool[key])
            fmt.sbprintf(&b, "[%2d] ", val)
        } else {
            strings.write_string(&b, "[  ] ")
        }
    }
    strings.write_byte(&b, '\n')

    for key in 0 ..< entity_sys.pool.max_key {
        if !entity_alive(key) {
            val := u32(entity_sys.pool.slot_pool[key])
            fmt.sbprintf(&b, "[%2d] ", val)
        } else {
            strings.write_string(&b, "[  ] ")
        }
    }
    strings.write_byte(&b, '\n')

    core.log_info("%s", strings.to_string(b))
}
