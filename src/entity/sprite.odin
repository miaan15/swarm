package entity

import "core:math"
import "../engine/core"
import "../global"

sprite_profile :: struct {
    tex: u32,
    rect: [4]u32,
}

sprite :: struct {
    key: u32,

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

    pool: pool(sprite)
} = {}

// ================================================================================================
sprite_sys_init :: proc(profile_cap, sprite_cap: u32) {
    sprite_sys.profile_list = transmute([^]sprite_profile)core.arena_alloc(&global.omni_arena, profile_cap * size_of(sprite_profile))
    sprite_sys.profile_cap = profile_cap

    // stub
    sprite_sys.profile_len = 1
    sprite_sys.profile_list[0] = sprite_profile{tex = 0, rect = {0, 0, 32, 32}}

    pool_init(&sprite_sys.pool, sprite_cap)
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
    if sprite_sys.pool.len >= sprite_sys.pool.cap {
        core.log_error("sprite_create: too many sprite (%d) => stub", sprite_sys.pool.len)
        return 0, &sprite_sys.pool.data_list[0]
    }

    key, ptr := pool_create(&sprite_sys.pool)

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
    if key == 0 || key >= sprite_sys.pool.max_key {
        core.log_error("sprite_destroy: sprite [%d] invalid", key)
        return
    }
    if sprite_sys.pool.slot_pool[key] >= 0 {
        core.log_warn("sprite_destroy: sprite [%d] already dead", key)
        return
    }

    pool_destroy(&sprite_sys.pool, key)

    core.log_debug("destroyed sprite [%d]", key)
}

sprite_get :: proc(key: u32) -> ^sprite {
    if key == 0 || key >= sprite_sys.pool.max_key {
        core.log_error("sprite_get: sprite [%d] invalid => stub", key)
        return &sprite_sys.pool.data_list[0]
    }
    if sprite_sys.pool.slot_pool[key] >= 0 {
        core.log_warn("sprite_get: sprite [%d] is dead => stub", key)
        return &sprite_sys.pool.data_list[0]
    }

    return pool_get(&sprite_sys.pool, key)
}

sprite_alive :: proc(key: u32) -> bool {
    return pool_alive(&sprite_sys.pool, key)
}

// ================================================================================================
sprite_sys_update :: proc() {

}

sprite_sys_draw :: proc() {

}
