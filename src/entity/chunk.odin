package entity

import "core:fmt"
import "core:math"
import "core:strings"
import "../engine/core"
import "../global"

chunk_instance :: struct {
    key: u32,
    data: u32,
    center: [2]f32,
    extents: [2]f32,

    list_idx: [4]u32,
}

chunk_map_entry :: struct {
    alive: bool,
    pos: [2]i32,

    instance_list: [^]u32,
    ins_len: u32,
}
chunk_mng :: struct {
    chunk_size: f32,
    chunk_max_instance: u32,

    instance_pool: pool(chunk_instance),

    chunk_map: [^]chunk_map_entry,
    cap, len: u32
}

// ================================================================================================
chunk_mng_init :: proc(mng: ^chunk_mng, chunk_size: f32, chunk_max_instance: u32, cap: u32) {
    mng.chunk_size = chunk_size
    mng.chunk_max_instance = chunk_max_instance

    pool_init(&mng.instance_pool, cap)
    mng.chunk_map = transmute([^]chunk_map_entry)core.arena_alloc(&global.omni_arena, cap * size_of(chunk_map_entry))
    mng.cap = cap

    // stub
    mng.len = 1
}

// ================================================================================================
chunk_mng_create :: proc(mng: ^chunk_mng, data: u32, center: [2]f32, extents: [2]f32) -> (_key: u32, _ptr: ^chunk_instance) {
    if mng.len >= mng.cap {
        core.log_error("chunk_mng_create: too many chunk points (%d) => stub", mng.len)
        return 0, &mng.instance_pool.data_list[0]
    }

    key, ptr := pool_create(&mng.instance_pool)
    ptr.data = data
    ptr.key = key
    ptr.center = center
    ptr.extents = extents

    DX: [4]f32 = { -1,  1, -1,  1 }
    DY: [4]f32 = { -1, -1,  1,  1 }
    visited_chunk_pos: [4][2]i32

    for i in 0..<4 {
        corner_pos := [2]f32{ center[0] + extents[0] * DX[i], center[1] + extents[1] * DY[i] }

        chunk_pos := _chunk_pos_cal(mng, corner_pos)
        visited_chunk_pos[i] = chunk_pos

        // check if already visited
        visited := false
        for j in 0..<i {
            if chunk_pos == visited_chunk_pos[j] {
                ptr.list_idx[i] = ptr.list_idx[j]
                visited = true
            }
        }

        // add to chunk
        if !visited {
            chunk_idx, chunk_ptr := _chunk_map_open(mng, chunk_pos)

            if chunk_ptr.ins_len >= mng.chunk_max_instance {
                core.log_error("chunk [%d %d] instances exceed mng.chunk_max_instance (%d)", chunk_pos[0], chunk_pos[1], mng.chunk_max_instance)
                pool_destroy(&mng.instance_pool, key)
                return
            }

            chunk_ptr.instance_list[chunk_ptr.ins_len] = key
            ptr.list_idx[i] = chunk_ptr.ins_len

            chunk_ptr.ins_len += 1
        }
    }

    return key, ptr
}

chunk_mng_destroy :: proc(mng: ^chunk_mng, key: u32) {
    if !pool_alive(&mng.instance_pool, key) {
        core.log_error("chunk_mng_destroy: instance [%d] invalid (dead or worse)", key)
        return
    }

    ptr := pool_get(&mng.instance_pool, key)

    DX: [4]f32 = { -1,  1, -1,  1 }
    DY: [4]f32 = { -1, -1,  1,  1 }
    visited_chunk_pos: [4][2]i32

    for i in 0..<4 {
        corner_pos := [2]f32{ ptr.center[0] + ptr.extents[0] * DX[i], ptr.center[1] + ptr.extents[1] * DY[i] }

        chunk_pos := _chunk_pos_cal(mng, corner_pos)
        visited_chunk_pos[i] = chunk_pos

        // check if already visited
        visited := false
        for j in 0..<i {
            if chunk_pos == visited_chunk_pos[j] {
                visited = true
            }
        }

        // remove from chunk
        if !visited {
            chunk_idx, chunk_ptr := _chunk_map_open(mng, chunk_pos)

            // replace removed <-> last
            removed_slot := ptr.list_idx[i]
            repl_slot := chunk_ptr.ins_len - 1
            assert(chunk_ptr.ins_len > 0 && removed_slot <= repl_slot)

            if removed_slot != repl_slot {
                chunk_ptr.instance_list[removed_slot] = chunk_ptr.instance_list[repl_slot]

                repl_ptr := pool_get(&mng.instance_pool, chunk_ptr.instance_list[repl_slot])
                for j in 0 ..< 4 {
                    repl_corner := [2]f32{
                        repl_ptr.center[0] + repl_ptr.extents[0] * DX[j],
                        repl_ptr.center[1] + repl_ptr.extents[1] * DY[j],
                    }
                    if _chunk_pos_cal(mng, repl_corner) == chunk_pos {
                        repl_ptr.list_idx[j] = removed_slot
                    }
                }
            }

            chunk_ptr.ins_len -= 1
        }
    }

    pool_destroy(&mng.instance_pool, key)
}

chunk_mng_update :: proc(mng: ^chunk_mng, key: u32, new_center: [2]f32, new_extents: [2]f32) {
    if !pool_alive(&mng.instance_pool, key) {
        core.log_error("chunk_mng_update: instance [%d] invalid (dead or worse)", key)
        return
    }

    ptr := pool_get(&mng.instance_pool, key)

    DX: [4]f32 = { -1,  1, -1,  1 }
    DY: [4]f32 = { -1, -1,  1,  1 }
    visited_chunk_pos: [4][2]i32

    // remove old
    for i in 0..<4 {
        corner_pos := [2]f32{ ptr.center[0] + ptr.extents[0] * DX[i], ptr.center[1] + ptr.extents[1] * DY[i] }

        chunk_pos := _chunk_pos_cal(mng, corner_pos)
        visited_chunk_pos[i] = chunk_pos

        // check if already visited
        visited := false
        for j in 0..<i {
            if chunk_pos == visited_chunk_pos[j] {
                visited = true
            }
        }

        // remove from chunk
        if !visited {
            chunk_idx, chunk_ptr := _chunk_map_open(mng, chunk_pos)

            // replace removed <-> last
            removed_slot := ptr.list_idx[i]
            repl_slot := chunk_ptr.ins_len - 1
            assert(chunk_ptr.ins_len > 0 && removed_slot <= repl_slot)

            if removed_slot != repl_slot {
                chunk_ptr.instance_list[removed_slot] = chunk_ptr.instance_list[repl_slot]

                repl_ptr := pool_get(&mng.instance_pool, chunk_ptr.instance_list[repl_slot])
                for j in 0 ..< 4 {
                    repl_corner := [2]f32{
                        repl_ptr.center[0] + repl_ptr.extents[0] * DX[j],
                        repl_ptr.center[1] + repl_ptr.extents[1] * DY[j],
                    }
                    if _chunk_pos_cal(mng, repl_corner) == chunk_pos {
                        repl_ptr.list_idx[j] = removed_slot
                    }
                }
            }

            chunk_ptr.ins_len -= 1
        }
    }

    // add new
    for i in 0..<4 {
        corner_pos := [2]f32{ new_center[0] + new_extents[0] * DX[i], new_center[1] + new_extents[1] * DY[i] }

        chunk_pos := _chunk_pos_cal(mng, corner_pos)
        visited_chunk_pos[i] = chunk_pos

        // check if already visited
        visited := false
        for j in 0..<i {
            if chunk_pos == visited_chunk_pos[j] {
                ptr.list_idx[i] = ptr.list_idx[j]
                visited = true
            }
        }

        // add to chunk
        if !visited {
            chunk_idx, chunk_ptr := _chunk_map_open(mng, chunk_pos)

            if chunk_ptr.ins_len >= mng.chunk_max_instance {
                core.log_error("chunk [%d %d] instances exceed mng.chunk_max_instance (%d)", chunk_pos[0], chunk_pos[1], mng.chunk_max_instance)
                pool_destroy(&mng.instance_pool, key)
                return
            }

            chunk_ptr.instance_list[chunk_ptr.ins_len] = key
            ptr.list_idx[i] = chunk_ptr.ins_len

            chunk_ptr.ins_len += 1
        }
    }

    ptr.center = new_center
    ptr.extents = new_extents
}

chunk_mng_get :: proc(mng: ^chunk_mng, key: u32) -> ^chunk_instance {
    if !pool_alive(&mng.instance_pool, key) {
        core.log_error("chunk_mng_get: instance [%d] invalid (dead or worse) => stub", key)
        return &mng.instance_pool.data_list[0]
    }
    return pool_get(&mng.instance_pool, key)
}

// chunk_mng_query :: proc(mng: ^chunk_mng, rect: [4]f32) -> (queried_points: ^[^]u32, queried_len: ^u32) {
//
// }

// PRIVATE
// ================================================================================================
_chunk_pos_cal :: proc(mng: ^chunk_mng, world_pos: [2]f32) -> [2]i32 {
    return { i32(math.floor(world_pos[0] / mng.chunk_size)), i32(math.floor(world_pos[1] / mng.chunk_size)) }
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
            ptr.instance_list = transmute([^]u32)core.arena_alloc(&global.omni_arena, mng.chunk_max_instance)
            ptr.ins_len = 0
            break;
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
    if !_pool_validate(&mng.instance_pool) {
        core.log_trace("chunk_mng validate: ins_pool failed validation")
        return false
    }

    // validate instances
    DX: [4]f32 = { -1,  1, -1,  1 }
    DY: [4]f32 = { -1, -1,  1,  1 }
    _idx: u32 = 0
    for ins_key, ins_ptr in pool_iterate(&mng.instance_pool, &_idx) {
        for i in 0 ..< 4 {
            corner_pos := [2]f32{
                ins_ptr.center[0] + ins_ptr.extents[0] * DX[i],
                ins_ptr.center[1] + ins_ptr.extents[1] * DY[i],
            }
            c_pos := _chunk_pos_cal(mng, corner_pos)

            chunk_found := false
            for c_idx in 1 ..< mng.cap {
                entry := &mng.chunk_map[c_idx]
                if entry.alive && entry.pos == c_pos {
                    chunk_found = true
                    slot_idx := ins_ptr.list_idx[i]

                    if slot_idx >= entry.ins_len {
                        core.log_trace("chunk_mng validate: instance [%d] corner %d list_idx (%d) >= ins_len (%d) in chunk [%d %d]",
                            ins_key, i, slot_idx, entry.ins_len, c_pos[0], c_pos[1])
                        return false
                    }

                    if entry.instance_list[slot_idx] != ins_key {
                        core.log_trace("chunk_mng validate: instance [%d] slot mismatch in chunk [%d %d] at slot %d (found [%d])",
                            ins_key, c_pos[0], c_pos[1], slot_idx, entry.instance_list[slot_idx])
                        return false
                    }
                    break
                }
            }

            if !chunk_found {
                core.log_trace("chunk_mng validate: instance [%d] corner %d references unopened chunk [%d %d]",
                    ins_key, i, c_pos[0], c_pos[1])
                return false
            }
        }
    }

    // validate chunks
    for c_idx in 1 ..< mng.cap {
        entry := &mng.chunk_map[c_idx]
        if !entry.alive {
            continue
        }

        for s_idx in 0 ..< entry.ins_len {
            key := entry.instance_list[s_idx]
            if !pool_alive(&mng.instance_pool, key) {
                core.log_trace("chunk_mng validate: chunk [%d %d] slot %d contains dead instance [%d]", entry.pos[0], entry.pos[1], s_idx, key)
                return false
            }

            ptr := pool_get(&mng.instance_pool, key)
            matched := false
            for i in 0 ..< 4 {
                corner_pos := [2]f32{
                    ptr.center[0] + ptr.extents[0] * DX[i],
                    ptr.center[1] + ptr.extents[1] * DY[i],
                }
                if _chunk_pos_cal(mng, corner_pos) == entry.pos && ptr.list_idx[i] == s_idx {
                    matched = true
                    break
                }
            }

            if !matched {
                core.log_trace("chunk_mng validate: instance [%d] does not reference chunk [%d %d] slot %d", key, entry.pos[0], entry.pos[1], s_idx)
                return false
            }
        }
    }

    return true
}

_chunk_mng_debug_log :: proc(mng: ^chunk_mng) {
    b := strings.builder_make(context.temp_allocator)

    fmt.sbprintf(&b, "--- CHUNK MNG DEBUG: len: %d; cap: %d; pool_len: %d; pool_cap: %d ---\n",
        mng.len, mng.cap, mng.instance_pool.len, mng.instance_pool.cap)
    fmt.sbprintf(&b, "STATUS: %s\n", "ok" if _chunk_mng_validate(mng) else "INVALID*")

    DX: [4]f32 = { -1,  1, -1,  1 }
    DY: [4]f32 = { -1, -1,  1,  1 }

    for c_idx in 1 ..< mng.cap {
        entry := &mng.chunk_map[c_idx]
        if !entry.alive {
            continue
        }

        fmt.sbprintf(&b, "chunk [%d %d] has %d instances:\n", entry.pos[0], entry.pos[1], entry.ins_len)

        for s_idx in 0 ..< entry.ins_len {
            key := entry.instance_list[s_idx]
            if !pool_alive(&mng.instance_pool, key) {
                fmt.sbprintf(&b, "- instance [%d]: <DEAD/INVALID>\n", key)
                continue
            }

            ptr := pool_get(&mng.instance_pool, key)

            c0 := _chunk_pos_cal(mng, { ptr.center[0] + ptr.extents[0] * DX[0], ptr.center[1] + ptr.extents[1] * DY[0] })
            c1 := _chunk_pos_cal(mng, { ptr.center[0] + ptr.extents[0] * DX[1], ptr.center[1] + ptr.extents[1] * DY[1] })
            c2 := _chunk_pos_cal(mng, { ptr.center[0] + ptr.extents[0] * DX[2], ptr.center[1] + ptr.extents[1] * DY[2] })
            c3 := _chunk_pos_cal(mng, { ptr.center[0] + ptr.extents[0] * DX[3], ptr.center[1] + ptr.extents[1] * DY[3] })

            fmt.sbprintf(
                &b,
                "- instance [%d]: data = [%d]; center = (%.1f %.1f); extents = (%.1f %.1f) in chunk [%d %d] [%d %d] [%d %d] [%d %d]\n",
                key,
                ptr.data,
                ptr.center[0], ptr.center[1],
                ptr.extents[0], ptr.extents[1],
                c0[0], c0[1],
                c1[0], c1[1],
                c2[0], c2[1],
                c3[0], c3[1],
            )
        }
    }

    core.log_info("%s", strings.to_string(b))
}
