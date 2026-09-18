package entity

import "core:fmt"
import "core:strings"
import "core:mem"
import "core:reflect"
import "../engine/core"
import "../global"

pool :: struct($T: typeid) {
    slot_pool: [^]i32,
    data_list: [^]T,
    cap, head, max_key, len: u32,

    data_field_key_offset: uintptr
}

pool_init :: proc(pool: ^pool($T), cap: u32) {
    field_key := reflect.struct_field_by_name(T, "key")
    if field_key.type == nil || field_key.type.id != u32 {
        core.log_error("pool T require field .key: u32")
        return
    }
    pool.data_field_key_offset = field_key.offset

    pool.slot_pool = transmute([^]i32)core.arena_alloc(&global.omni_arena, cap * size_of(i32))
    pool.data_list = transmute([^]T)core.arena_alloc(&global.omni_arena, cap * size_of(T))
    pool.cap = cap

    // stub
    pool.head = 1
    pool.max_key = 1
    pool.len = 1
}

pool_create :: proc(pool: ^pool($T)) -> (_key: u32, _ptr: ^T) {
    if pool.len >= pool.cap {
        return 0, &pool.data_list[0]
    }

    key := pool.head
    if key == pool.max_key {
        pool.max_key += 1
        pool.head += 1
    } else {
        pool.head = u32(pool.slot_pool[key])
    }
    pool.slot_pool[key] = -i32(pool.len)

    assert(pool.len < pool.cap)
    ptr := &pool.data_list[pool.len]
    pool.len += 1

    mem.zero(ptr, size_of(T))
    mem.copy(rawptr(uintptr(ptr) + pool.data_field_key_offset), &key, size_of(u32))

    return key, ptr
}

pool_destroy :: proc(pool: ^pool($T), key: u32) -> bool {
    if key == 0 || key >= pool.max_key { return false }
    if pool.slot_pool[key] >= 0 { return false }

    idx := u32(-pool.slot_pool[key])

    pool.slot_pool[key] = i32(pool.head)
    pool.head = key

    assert(idx < pool.len)
    if idx != pool.len - 1 {
        del_ptr := &pool.data_list[idx]
        repl_ptr := &pool.data_list[pool.len - 1]
        repl_key: u32
        mem.copy(&repl_key, rawptr(uintptr(repl_ptr) + pool.data_field_key_offset), size_of(u32))
        assert(repl_key < pool.max_key)

        mem.copy(del_ptr, repl_ptr, size_of(T))
        mem.copy(rawptr(uintptr(del_ptr) + pool.data_field_key_offset), &repl_key, size_of(u32))
        pool.slot_pool[repl_key] = -i32(idx)
    }
    pool.len -= 1

    return true
}

pool_get :: proc(pool: ^pool($T), key: u32) -> ^T {
    if key == 0 || key >= pool.max_key {
        return &pool.data_list[0]
    }
    if pool.slot_pool[key] >= 0 {
        return &pool.data_list[0]
    }

    idx := -pool.slot_pool[key]
    assert(idx > 0 && idx < i32(pool.len))
    return &pool.data_list[idx]
}

pool_alive :: proc(pool: ^pool($T), key: u32) -> bool {
    if key == 0 || key >= pool.max_key { return false }

    assert(pool.slot_pool[key] > -i32(pool.len))
    return pool.slot_pool[key] < 0
}

// TEST
// ================================================================================================
_pool_validate :: proc(p: ^pool($T)) -> bool {
    if p.len > p.cap || p.max_key > p.cap || p.len == 0 {
        core.log_trace("pool validate: invalid bounds (len=%d, cap=%d, max_key=%d)", p.len, p.cap, p.max_key)
        return false
    }

    cnt_alive: u32 = 0
    max_idx: u32 = 0

    for key in 1 ..< p.max_key {
        if !pool_alive(p, key) {
            continue
        }

        cnt_alive += 1
        idx := u32(-p.slot_pool[key])

        if idx == 0 || idx >= p.len {
            core.log_trace("pool validate: key [%d] slot points out of range (%d)", key, p.slot_pool[key])
            return false
        }

        data_key: u32
        mem.copy(&data_key, rawptr(uintptr(&p.data_list[idx]) + p.data_field_key_offset), size_of(u32))
        if data_key != key {
            core.log_trace("pool validate: key mismatch at idx %d (expected %d, found %d)", idx, key, data_key)
            return false
        }

        if idx > max_idx {
            max_idx = idx
        }
    }

    if cnt_alive != p.len - 1 {
        core.log_trace("pool validate: alive count = %d, expected %d", cnt_alive, p.len - 1)
        return false
    }

    if cnt_alive > 0 && max_idx != p.len - 1 {
        core.log_trace("pool validate: max packed idx = %d, expected %d", max_idx, p.len - 1)
        return false
    }

    visited := make([]bool, p.max_key, context.temp_allocator)
    curr := p.head
    for curr != 0 && curr < p.max_key {
        if visited[curr] {
            core.log_trace("pool validate: cycle detected in free list at key [%d]", curr)
            return false
        }
        visited[curr] = true

        next := p.slot_pool[curr]
        if next < 0 {
            core.log_trace("pool validate: active slot found inside free list at key [%d]", curr)
            return false
        }
        curr = u32(next)
    }

    return true
}

_pool_debug_log :: proc(p: ^pool($T)) {
    b := strings.builder_make(context.temp_allocator)

    fmt.sbprintf(&b, "--- POOL DEBUG [len: %d, max_key: %d, cap: %d, head: %d] ---\n", p.len, p.max_key, p.cap, p.head)
    fmt.sbprintf(&b, "STATUS: %s\n", "ok" if _pool_validate(p) else "ERROR")

    // Keys row
    fmt.sbprint(&b, "KEYS: ")
    for key in 0 ..< p.max_key {
        fmt.sbprintf(&b, " %2d  ", key)
    }
    strings.write_byte(&b, '\n')

    fmt.sbprint(&b, "ACTV: ")
    for key in 0 ..< p.max_key {
        if pool_alive(p, key) {
            fmt.sbprintf(&b, "[%2d] ", u32(-p.slot_pool[key]))
        } else {
            strings.write_string(&b, "[  ] ")
        }
    }
    strings.write_byte(&b, '\n')

    fmt.sbprint(&b, "FREE: ")
    for key in 0 ..< p.max_key {
        if !pool_alive(p, key) {
            fmt.sbprintf(&b, "[%2d] ", u32(p.slot_pool[key]))
        } else {
            strings.write_string(&b, "[  ] ")
        }
    }
    strings.write_byte(&b, '\n')

    core.log_info("%s", strings.to_string(b))
}
