package entity

import "core:mem"
import "core:fmt"
import "core:math"
import "core:strings"
import "../engine/core"
import "../global"

chunk_instance :: struct {
    key: u32,
    data: u32,
    center: [2]f32,

    links: [2]u32,
    chunk_pos: [2]i32,
}

chunk_map_entry :: struct {
    alive: bool,
    pos: [2]i32,

    ins_begin, ins_len: u32,
}
chunk_mng :: struct {
    chunk_size: f32,

    ins_pool: pool(chunk_instance),

    chunk_map: [^]chunk_map_entry,
    cap, len: u32
}

// ================================================================================================
chunk_mng_init :: proc(mng: ^chunk_mng, chunk_size: f32, cap: u32) {
    mng.chunk_size = chunk_size

    pool_init(&mng.ins_pool, cap)
    mng.chunk_map = transmute([^]chunk_map_entry)core.arena_alloc(&global.omni_arena, cap * size_of(chunk_map_entry))
    mng.cap = cap

    // stub
    mng.len = 1
}

// ================================================================================================
chunk_mng_create :: proc(mng: ^chunk_mng, data: u32, center: [2]f32) -> (_key: u32, _ptr: ^chunk_instance) {
    if mng.len >= mng.cap {
        core.log_error("chunk_mng_create: too many chunk points (%d) => stub", mng.len)
        return 0, &mng.ins_pool.data_list[0]
    }

    ins_key, ins_ptr := pool_create(&mng.ins_pool)
    ins_ptr.data = data
    ins_ptr.key = ins_key
    ins_ptr.center = center

    ins_ptr.chunk_pos = _chunk_pos_cal(mng, center)

    chunk_idx, chunk_ptr := _chunk_map_open(mng, ins_ptr.chunk_pos)

    // linking
    ins_ptr.links[0] = 0
    ins_ptr.links[1] = chunk_ptr.ins_begin

    if chunk_ptr.ins_begin != 0 {
        assert(chunk_ptr.ins_begin < mng.ins_pool.max_key)
        assert(pool_get(&mng.ins_pool, chunk_ptr.ins_begin).chunk_pos == ins_ptr.chunk_pos)
        pool_get(&mng.ins_pool, chunk_ptr.ins_begin).links[0] = ins_key
    }

    chunk_ptr.ins_begin = ins_key

    chunk_ptr.ins_len += 1

    return ins_key, ins_ptr
}

chunk_mng_destroy :: proc(mng: ^chunk_mng, key: u32) {
    if !pool_alive(&mng.ins_pool, key) {
        core.log_error("chunk_mng_destroy: instance [%d] invalid (dead or worse)", key)
        return
    }

    ins_ptr := pool_get(&mng.ins_pool, key)
    chunk_idx, chunk_ptr := _chunk_map_open(mng, ins_ptr.chunk_pos)

    if ins_ptr.links[0] != 0 {
        assert(ins_ptr.links[0] < mng.ins_pool.max_key)
        assert(pool_get(&mng.ins_pool, ins_ptr.links[0]).chunk_pos == ins_ptr.chunk_pos)
        pool_get(&mng.ins_pool, ins_ptr.links[0]).links[1] = ins_ptr.links[1]
    }

    if ins_ptr.links[1] != 0 {
        assert(ins_ptr.links[1] < mng.ins_pool.max_key)
        assert(pool_get(&mng.ins_pool, ins_ptr.links[1]).chunk_pos == ins_ptr.chunk_pos)
        pool_get(&mng.ins_pool, ins_ptr.links[1]).links[0] = ins_ptr.links[0]
    }

    if chunk_ptr.ins_begin == key { chunk_ptr.ins_begin = ins_ptr.links[1] }

    chunk_ptr.ins_len -= 1

    pool_destroy(&mng.ins_pool, key)
}

chunk_mng_update :: proc(mng: ^chunk_mng, key: u32, new_center: [2]f32) {
    if !pool_alive(&mng.ins_pool, key) {
        core.log_error("chunk_mng_update: instance [%d] invalid (dead or worse)", key)
        return
    }

    ins_ptr := pool_get(&mng.ins_pool, key)

    new_chunk_pos := _chunk_pos_cal(mng, new_center)
    if ins_ptr.chunk_pos == new_chunk_pos { return }

    { // remove old
        chunk_idx, chunk_ptr := _chunk_map_open(mng, ins_ptr.chunk_pos)

        if ins_ptr.links[0] != 0 {
            assert(ins_ptr.links[0] < mng.ins_pool.max_key)
            assert(pool_get(&mng.ins_pool, ins_ptr.links[0]).chunk_pos == ins_ptr.chunk_pos)
            pool_get(&mng.ins_pool, ins_ptr.links[0]).links[1] = ins_ptr.links[1]
        }

        if ins_ptr.links[1] != 0 {
            assert(ins_ptr.links[1] < mng.ins_pool.max_key)
            assert(pool_get(&mng.ins_pool, ins_ptr.links[1]).chunk_pos == ins_ptr.chunk_pos)
            pool_get(&mng.ins_pool, ins_ptr.links[1]).links[0] = ins_ptr.links[0]
        }

        if chunk_ptr.ins_begin == key { chunk_ptr.ins_begin = ins_ptr.links[1] }

        chunk_ptr.ins_len -= 1
    }

    ins_ptr.center = new_center
    ins_ptr.chunk_pos = new_chunk_pos

    { // add new
        chunk_idx, chunk_ptr := _chunk_map_open(mng, ins_ptr.chunk_pos)

        ins_ptr.links[0] = 0
        ins_ptr.links[1] = chunk_ptr.ins_begin

        if chunk_ptr.ins_begin != 0 {
            assert(chunk_ptr.ins_begin < mng.ins_pool.max_key)
            assert(pool_get(&mng.ins_pool, chunk_ptr.ins_begin).chunk_pos == ins_ptr.chunk_pos)
            pool_get(&mng.ins_pool, chunk_ptr.ins_begin).links[0] = key
        }

        chunk_ptr.ins_begin = key

        chunk_ptr.ins_len += 1
    }
}

chunk_mng_get :: proc(mng: ^chunk_mng, key: u32) -> ^chunk_instance {
    if !pool_alive(&mng.ins_pool, key) {
        core.log_error("chunk_mng_get: instance [%d] invalid (dead or worse) => stub", key)
        return &mng.ins_pool.data_list[0]
    }
    return pool_get(&mng.ins_pool, key)
}

chunk_mng_query :: proc(mng: ^chunk_mng, rect: [4]f32, arena: ^core.arena = global.tick_arena) -> (_queried_points: [^]u32, _queried_len: u32) {
    queried_points : [^]u32 = nil
    queried_len, queried_cap: u32 = 0, 0

    padding := ENTITY_MAX_BOUNDS_SIZE / 2
    min := [2]f32{ rect[0] - padding, rect[1] - padding }
    max := [2]f32{ rect[0] + rect[2] + padding, rect[1] + rect[3] + padding }

    min_chunk := _chunk_pos_cal(mng, { min[0], min[1] })
    max_chunk := _chunk_pos_cal(mng, { max[0], max[1] })

    for cy in min_chunk[1] ..= max_chunk[1] {
        for cx in min_chunk[0] ..= max_chunk[0] {
            chunk_idx, chunk_ptr := _chunk_map_open(mng, [2]i32{cx, cy})

            ins_key := chunk_ptr.ins_begin
            for ins_key != 0 {
                ins_ptr := pool_get(&mng.ins_pool, ins_key)

                if ins_ptr.center[0] >= min[0] && ins_ptr.center[0] <= max[0] &&
                   ins_ptr.center[1] >= min[1] && ins_ptr.center[1] <= max[1] {
                    if queried_len >= queried_cap {
                        queried_cap = queried_cap < 2 ? 2 : queried_cap * 4
                        _new_ptr := transmute([^]u32)core.arena_alloc_raw(arena, queried_cap)
                        if queried_points != nil { mem.copy(_new_ptr, queried_points, int(queried_len) * size_of(u32)) }
                        queried_points = _new_ptr
                    }

                    queried_points[queried_len] = ins_key
                    queried_len += 1
                }

                ins_key = ins_ptr.links[1]
            }
        }
    }

    return queried_points, queried_len
}

// UTIL
// ================================================================================================
chunk_cal_center_rect :: proc(rect: [4]f32) -> [2]f32 {
    return { rect[0] + rect[2] / 2, rect[1] + rect[3] / 2 }
}

// PRIVATE
// ================================================================================================
_chunk_pos_cal :: proc(mng: ^chunk_mng, world_pos: [2]f32) -> [2]i32 {
    return { i32(math.floor(world_pos[0] / mng.chunk_size)), i32(math.floor(world_pos[1] / mng.chunk_size)) }
}

_chunk_instance_in_single_chunk :: proc(mng: ^chunk_mng, center: [2]f32, extents: [2]f32) -> (_chunk_pos: [2]i32, ok: bool) {
    tl_chunk_pos := _chunk_pos_cal(mng, center - extents)
    br_chunk_pos := _chunk_pos_cal(mng, center + extents)
    if tl_chunk_pos == br_chunk_pos { return tl_chunk_pos, true }
    return tl_chunk_pos, false
}

_chunk_map_open :: proc(mng: ^chunk_mng, pos: [2]i32) -> (_idx: u32, _ptr: ^chunk_map_entry) {
    // some kind of algorithm idk
    hash := (u64(transmute(u32)pos[0]) << 32) | u64(transmute(u32)pos[1])
    hash ~= hash >> 30
    hash *= 0xbf58476d1ce4e5b9
    hash ~= hash >> 27
    hash *= 0x94d049bb133111eb
    hash ~= hash >> 31

    first_idx := u32(hash % u64(mng.cap - 1)) + 1

    idx := first_idx
    for {
        entry := mng.chunk_map[idx]
        if !entry.alive {
            ptr := &mng.chunk_map[idx]
            ptr.alive = true
            ptr.pos = pos
            break
        }
        if entry.pos == pos { break; }

        idx += 1
        if idx >= mng.cap { idx = 1 }

        if idx == first_idx {
            core.log_error("too much chunks opened (%d) => stub", mng.cap)
            return 0, &mng.chunk_map[0]
        }
    }

    return idx, &mng.chunk_map[idx]
}

// TEST
// ================================================================================================
_chunk_mng_validate :: proc(mng: ^chunk_mng) -> bool {
    if !_pool_validate(&mng.ins_pool) {
        core.log_trace("chunk_mng validate: instance_pool failed validation")
        return false
    }

    total_active_instances: u32 = 0
    _idx: u32 = 0
    for ins_key, ins_ptr in pool_iterate(&mng.ins_pool, &_idx) {
        total_active_instances += 1

        expected_chunk_pos := _chunk_pos_cal(mng, ins_ptr.center)
        if ins_ptr.chunk_pos != expected_chunk_pos {
            core.log_trace("chunk_mng validate: instance [%d] pos mismatch: stored [%d %d] vs computed [%d %d]",
                ins_key, ins_ptr.chunk_pos[0], ins_ptr.chunk_pos[1], expected_chunk_pos[0], expected_chunk_pos[1])
            return false
        }

        // prev link
        prev_key := ins_ptr.links[0]
        if prev_key != 0 {
            if !pool_alive(&mng.ins_pool, prev_key) {
                core.log_trace("chunk_mng validate: instance [%d] prev link points to dead key [%d]", ins_key, prev_key)
                return false
            }
            prev_ptr := pool_get(&mng.ins_pool, prev_key)
            if prev_ptr.links[1] != ins_key {
                core.log_trace("chunk_mng validate: link break: [%d].links[0] = [%d], but [%d].links[1] = [%d]",
                    ins_key, prev_key, prev_key, prev_ptr.links[1])
                return false
            }
            if prev_ptr.chunk_pos != ins_ptr.chunk_pos {
                core.log_trace("chunk_mng validate: adjacent instances [%d] and [%d] have different chunk positions", ins_key, prev_key)
                return false
            }
        }

        // next
        next_key := ins_ptr.links[1]
        if next_key != 0 {
            if !pool_alive(&mng.ins_pool, next_key) {
                core.log_trace("chunk_mng validate: instance [%d] next link points to dead key [%d]", ins_key, next_key)
                return false
            }
            next_ptr := pool_get(&mng.ins_pool, next_key)
            if next_ptr.links[0] != ins_key {
                core.log_trace("chunk_mng validate: link break: [%d].links[1] = [%d], but [%d].links[0] = [%d]",
                    ins_key, next_key, next_key, next_ptr.links[0])
                return false
            }
            if next_ptr.chunk_pos != ins_ptr.chunk_pos {
                core.log_trace("chunk_mng validate: adjacent instances [%d] and [%d] have different chunk positions", ins_key, next_key)
                return false
            }
        }
    }

    //
    total_chunk_instances: u32 = 0
    visited := make([]bool, mng.ins_pool.max_key, context.temp_allocator)

    for c_idx in 1 ..< mng.cap {
        entry := &mng.chunk_map[c_idx]
        if !entry.alive {
            continue
        }

        curr := entry.ins_begin
        chunk_count: u32 = 0

        if curr != 0 {
            first_ptr := pool_get(&mng.ins_pool, curr)
            if first_ptr.links[0] != 0 {
                core.log_trace("chunk_mng validate: chunk [%d %d] head [%d] has non-zero prev link (%d)",
                    entry.pos[0], entry.pos[1], curr, first_ptr.links[0])
                return false
            }
        }

        for curr != 0 {
            if !pool_alive(&mng.ins_pool, curr) {
                core.log_trace("chunk_mng validate: chunk [%d %d] list contains dead instance [%d]",
                    entry.pos[0], entry.pos[1], curr)
                return false
            }

            if visited[curr] {
                core.log_trace("chunk_mng validate: cycle detected in chunk [%d %d] list at instance [%d]",
                    entry.pos[0], entry.pos[1], curr)
                return false
            }
            visited[curr] = true

            curr_ptr := pool_get(&mng.ins_pool, curr)
            if curr_ptr.chunk_pos != entry.pos {
                core.log_trace("chunk_mng validate: instance [%d] chunk_pos [%d %d] does not match chunk [%d %d]",
                    curr, curr_ptr.chunk_pos[0], curr_ptr.chunk_pos[1], entry.pos[0], entry.pos[1])
                return false
            }

            chunk_count += 1
            curr = curr_ptr.links[1]
        }

        if chunk_count != entry.ins_len {
            core.log_trace("chunk_mng validate: chunk [%d %d] counted %d instances but instance_len = %d",
                entry.pos[0], entry.pos[1], chunk_count, entry.ins_len)
            return false
        }

        total_chunk_instances += chunk_count
    }

    // no unlink
    if total_chunk_instances != total_active_instances {
        core.log_trace("chunk_mng validate: instances across chunks (%d) != pool active count (%d)",
            total_chunk_instances, total_active_instances)
        return false
    }

    return true
}

_chunk_mng_debug_log :: proc(mng: ^chunk_mng) {
    b := strings.builder_make(context.temp_allocator)

    fmt.sbprintf(&b, "--- CHUNK MNG DEBUG: len: %d; cap: %d; pool_len: %d; pool_cap: %d ---\n",
        mng.len, mng.cap, mng.ins_pool.len, mng.ins_pool.cap)
    fmt.sbprintf(&b, "STATUS: %s\n", "ok" if _chunk_mng_validate(mng) else "INVALID*")

    for c_idx in 1 ..< mng.cap {
        entry := &mng.chunk_map[c_idx]
        if !entry.alive {
            continue
        }

        fmt.sbprintf(&b, "chunk [%d %d] has %d instances:\n", entry.pos[0], entry.pos[1], entry.ins_len)

        curr := entry.ins_begin
        for curr != 0 {
            if !pool_alive(&mng.ins_pool, curr) {
                fmt.sbprintf(&b, "- instance [%d]: <DEAD/INVALID>\n", curr)
                break
            }

            ptr := pool_get(&mng.ins_pool, curr)
            fmt.sbprintf(
                &b,
                "- instance [%d]: data = [%d]; center = (%.1f, %.1f) in chunk [%d %d]\n",
                curr,
                ptr.data,
                ptr.center[0], ptr.center[1],
                ptr.chunk_pos[0], ptr.chunk_pos[1],
            )

            curr = ptr.links[1]
        }
    }

    core.log_info("%s", strings.to_string(b))
}
