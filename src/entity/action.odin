package entity

import "../engine/core"
import "../global"

action :: struct {
    from, to: u32,
    type: u32,
    data: [3]u32,
}

action_sys : struct {
    buffer: [^]action,
    cap, len: u32,
} = {}

action_sys_init :: proc(cap: u32) {
    action_sys.buffer = transmute([^]action)core.arena_alloc(&global.omni_arena, cap * size_of(action))
    action_sys.cap = cap
    action_sys.len = 0
}

action_make :: proc(from, to: u32, type: u32, data: [3]u32) {
    if action_sys.len >= action_sys.cap {
        core.log_error("action_make: too much actions (%u)", action_sys.len)
        return;
    }

    action_sys.buffer[action_sys.len] = action{from, to, type, data}
    action_sys.len += 1

    core.log_debug("make action: from [%u] to [%u]; type = [%u]; data = (%u %u %u)", from, to, type, data[0], data[1], data[2])
}

action_sys_update :: proc() {
    for i in 0..<action_sys.len {

    }

    action_sys.len = 0
}
