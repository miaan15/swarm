package entity

import "core:math"
import "core:mem"
import "../engine/core"
import "../global"

sprite_profile :: struct {
    tex: u32,
    rect: [4]u32,
}

sprite :: struct {
    idx: u32,

    tex: u32,
    src, dest: [4]f32,
    z: i8,

    last_tick_pos: [2]f32,
    interpolate_pos:[2]f32,

    ett_owner: u32,
    ett_links: [2]u32,

    ett_offset: [2]f32,
    ett_z: i8,
}

sprite_sys : struct {
    profile_list: [^]sprite_profile,
    profile_cap, profile_len: u32,

    slot_pool: [^]i32,
    list: [^]sprite,
    cap, head, max_key, len: u32,
} = {}

// ================================================================================================
sprite_sys_init :: proc(profile_cap, sprite_cap: u32) {
    sprite_sys.profile_list = transmute([^]sprite_profile)core.arena_alloc(&global.omni_arena, profile_cap * size_of(sprite_profile))
    sprite_sys.profile_cap = profile_cap

    sprite_sys.slot_pool = transmute([^]i32)core.arena_alloc(&global.omni_arena, sprite_cap * size_of(i32))
    sprite_sys.list = transmute([^]sprite)core.arena_alloc(&global.omni_arena, sprite_cap * size_of(sprite))
    sprite_sys.cap = sprite_cap

    // stub
    sprite_sys.profile_len = 1
    sprite_sys.profile_list[0] = sprite_profile{tex = 0, rect = {0, 0, 32, 32}}

    sprite_sys.head = 1
    sprite_sys.max_key = 1
    sprite_sys.len = 1
}

// ================================================================================================
sprite_profile_create :: proc(tex: u32, rect: [4]u32) -> u32 {
    if sprite_sys.profile_len >= sprite_sys.profile_cap {
        core.log_error("sprite_profile_create: too much sprite profiles (%d) => stub", sprite_sys.profile_len)
        return 0
    }

    idx := sprite_sys.profile_len
    sprite_sys.profile_list[idx] = sprite_profile{tex = tex, rect = rect}
    sprite_sys.profile_len += 1

    core.log_debug("created sprite profile [%d]: tex = %d, rect = (%d %d %d %d)", idx, tex, rect[0], rect[1], rect[2], rect[3])

    return idx
}

sprite_profile_get :: proc(idx: u32) -> sprite_profile {
    if idx == 0 || idx >= sprite_sys.profile_len {
        core.log_error("sprite_profile_get: profile [%d] invalid => stub", idx)
        return sprite_sys.profile_list[0]
    }
    return sprite_sys.profile_list[idx]
}

// ================================================================================================
sprite_create :: proc(profile_idx: u32, dest: [4]f32 = {0, 0, 0, 0}, z: i8 = 0) -> (_key: u32, _ptr: ^sprite) {
    if sprite_sys.len >= sprite_sys.cap {
        core.log_error("sprite_create: too many sprites (%d) => stub", sprite_sys.len)
        return 0, &sprite_sys.list[0]
    }

    if profile_idx == 0 || profile_idx >= sprite_sys.profile_len {
        core.log_error("sprite_create: profile [%d] invalid => stub", profile_idx)
        return 0, &sprite_sys.list[0]
    }

    key := sprite_sys.head
    if key == sprite_sys.max_key {
        sprite_sys.max_key += 1
        sprite_sys.head += 1
    } else {
        sprite_sys.head = u32(sprite_sys.slot_pool[key])
    }
    sprite_sys.slot_pool[key] = -i32(sprite_sys.len)

    assert(sprite_sys.len < sprite_sys.cap)
    ptr := &sprite_sys.list[sprite_sys.len]
    sprite_sys.len += 1

    mem.zero(ptr, size_of(sprite))
    ptr.idx = key

    profile := sprite_profile_get(profile_idx)
    ptr.tex = profile.tex
    ptr.src = { f32(profile.rect[0]), f32(profile.rect[1]), f32(profile.rect[2]), f32(profile.rect[3]) }
    ptr.dest = dest
    ptr.z = z
    ptr.interpolate_pos = { math.nan_f32(), math.nan_f32() }

    core.log_debug("created sprite [%d]: profile = [%d]; dest = [%.1f %.1f]; z = %d", key, dest[0], dest[1], dest[2], dest[3], z)

    return key, ptr
}

sprite_destroy :: proc(key: u32) {
    if key == 0 || key >= sprite_sys.max_key {
        core.log_error("sprite_destroy: sprite [%d] invalid", key)
        return
    }
    if sprite_sys.slot_pool[key] < 0 {
        core.log_warn("sprite_destroy: sprite [%d] already dead", key)
        return
    }
    idx := u32(-sprite_sys.slot_pool[key])

    sprite_sys.slot_pool[key] = i32(sprite_sys.head)
    sprite_sys.head = key

    assert(idx < sprite_sys.len)
    if (idx != sprite_sys.len - 1) {
        del_ptr := &sprite_sys.list[idx]
        repl_ptr := &sprite_sys.list[sprite_sys.len - 1]
        repl_key := repl_ptr.idx
        assert(repl_key < sprite_sys.max_key)

        mem.copy(del_ptr, repl_ptr, size_of(sprite))
        del_ptr.idx = repl_key
        sprite_sys.slot_pool[repl_key] = -i32(idx)
    }
    sprite_sys.len -= 1

    core.log_debug("destroyed sprite [%d]", key)
}

sprite_get :: proc(key: u32) -> ^sprite {
    if key == 0 || key >= sprite_sys.max_key {
        core.log_error("sprite_get: sprite [%d] invalid => stub", key)
        return &sprite_sys.list[0]
    }
    if sprite_sys.slot_pool[key] >= 0 {
        core.log_warn("sprite_get: sprite [%d] is dead => stub", key)
        return &sprite_sys.list[0]
    }

    idx := -sprite_sys.slot_pool[key]
    assert(idx > 0 && idx < i32(sprite_sys.len))
    return &sprite_sys.list[idx]
}

sprite_alive :: proc(key: u32) -> bool {
    if key == 0 || key >= sprite_sys.max_key { return false }

    assert(sprite_sys.slot_pool[key] > -i32(sprite_sys.len))
    return sprite_sys.slot_pool[key] < 0
}

// ================================================================================================
sprite_sys_update :: proc() {

}

sprite_sys_draw :: proc() {

}
