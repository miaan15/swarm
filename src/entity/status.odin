package entity

import "core:mem"
import "../engine/core"
import "../global"

ALIVE_POOL_FLAG :: 0xFFFFFFFF
status :: struct {
    pool_flag: u32,
    idx: u32,

    type: u32,

    ett_owner: u32,
    ett_links: [2]u32,

    //
    stack_cnt: u32,
    duration_ms, last_time_check_ms: u32,

    damage: f32,
}

status_sys : struct {
    pool: [^]status,
    cap, head, max_idx, len: u32,
} = {}

// ================================================================================================
status_sys_init :: proc(cap: u32) {
    status_sys.pool = transmute([^]status)core.arena_alloc(&global.omni_arena, cap * size_of(status))
    status_sys.cap = cap

    // stub
    status_sys.head = 1
    status_sys.max_idx = 1
    status_sys.len = 1
}

// ================================================================================================
status_create :: proc(type: u32) -> (_idx: u32, _ptr: ^status) {
    if status_sys.len >= status_sys.cap {
        core.log_error("status_create: too many statuses (%d) => stub", status_sys.len)
        return 0, &status_sys.pool[0]
    }

    idx := status_sys.head
    assert(idx <= status_sys.max_idx)
    if idx == status_sys.max_idx {
        status_sys.max_idx += 1
        status_sys.head += 1
    } else {
        status_sys.head = status_sys.pool[idx].pool_flag
    }

    status_sys.len += 1

    ptr := &status_sys.pool[idx]

    mem.zero(ptr, size_of(status))
    status_sys.pool[idx].pool_flag = ALIVE_POOL_FLAG
    ptr.idx = idx

    ptr.type = type

    core.log_debug("created status [%d]: type = %d", idx, type)

    return idx, ptr
}

status_destroy :: proc(idx: u32) {
    if status_sys.pool[idx].pool_flag != ALIVE_POOL_FLAG {
        core.log_warn("status_destroy: status [%d] already dead", idx)
        return
    }

    status_sys.pool[idx].pool_flag = status_sys.head
    status_sys.head = idx
    status_sys.len -= 1

    core.log_debug("destroyed status [%d]", idx)
}

status_get :: proc(idx: u32) -> ^status {
    if idx == 0 || idx >= status_sys.max_idx {
        core.log_error("status_get: status [%d] invalid => stub", idx)
        return &status_sys.pool[0]
    }

    if status_sys.pool[idx].pool_flag != ALIVE_POOL_FLAG {
        core.log_error("status_get: status [%d] is dead => stub", idx)
        return &status_sys.pool[0]
    }

    return &status_sys.pool[idx]
}

status_alive :: proc(idx: u32) -> bool {
    if idx == 0 || idx >= status_sys.max_idx { return false }
    return status_sys.pool[idx].pool_flag == ALIVE_POOL_FLAG
}
