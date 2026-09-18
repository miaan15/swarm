package entity

import "core:mem"
import "base:intrinsics"
import "core:reflect"
import "../engine/core"
import "../global"

has_u32_key :: proc($T: typeid) -> bool {
    when !intrinsics.type_is_struct(T) {
        return false
    } else {
        field := reflect.struct_field_by_name(T, "key")
        return field.type != nil && field.type.id == u32
    }
}

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
        return;
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
    if (idx != pool.len - 1) {
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

