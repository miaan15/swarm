package entity

import "core:math"
import "../engine/core"
import "../global"

chunk_point :: struct {
    key: u32,
    links: [2]u32,
    pos: [2]f32,
    data: u32,
}

chunk_map_entry :: struct {
    alive: bool,
    pos: [2]i32,

    point_begin, point_len: u32,
}
chunk_mng :: struct {
    chunk_size: f32,
    point_pool: pool(chunk_point),
    chunk_map: [^]chunk_map_entry,
    cap, len: u32
}

// ================================================================================================
chunk_mng_init :: proc(mng: ^chunk_mng, chunk_size: f32, cap: u32) {
    pool_init(&mng.point_pool, cap)
    mng.chunk_map = transmute([^]chunk_map_entry)core.arena_alloc(&global.omni_arena, cap * size_of(chunk_map_entry))
    mng.cap = cap

    // stub
    mng.len = 1
}

chunk_mng_create :: proc(mng: ^chunk_mng, pos: [2]f32, data: u32) -> (_key: u32, _ptr: ^chunk_point) {
    if mng.len >= mng.cap {
        core.log_error("chunk_mng_create: too many chunk points (%d) => stub", mng.len)
        return 0, &mng.point_pool.data_list[0]
    }

    key, ptr := pool_create(&mng.point_pool)

    chunk_pos := _chunk_pos_cal(mng.chunk_size, pos)
    chunk_idx, chunk_ptr := _chunk_map_open(mng, chunk_pos)

    ptr.links[1] = chunk_ptr.point_begin
    ptr.links[0] = 0

    if chunk_ptr.point_begin != 0 { pool_get(&mng.point_pool, chunk_ptr.point_begin).links[0] = key }

    chunk_ptr.point_begin = key
    chunk_ptr.point_len += 1

    return key, ptr
}

chunk_mng_destroy :: proc(mng: ^chunk_mng, key: u32) {
    if !pool_alive(&mng.point_pool, key) {
        core.log_error("chunk_mng_destroy: chunk point [%d] invalid (dead or worse)", key)
        return
    }

    ptr := pool_get(&mng.point_pool, key)

    chunk_pos := _chunk_pos_cal(mng.chunk_size, ptr.pos)
    chunk_idx, chunk_ptr := _chunk_map_open(mng, chunk_pos)

    if ptr.links[0] != 0 {
        assert(pool_alive(&mng.point_pool, ptr.links[0]))
        pool_get(&mng.point_pool, ptr.links[0]).links[1] = ptr.links[1]
    }

    if ptr.links[1] != 0 {
        assert(pool_alive(&mng.point_pool, ptr.links[1]))
        pool_get(&mng.point_pool, ptr.links[1]).links[0] = ptr.links[0]
    }

    ptr.links[0] = 0
    ptr.links[1] = 0

    if chunk_ptr.point_begin == key { chunk_ptr.point_begin = 0 }
    chunk_ptr.point_len -= 1
}

chunk_mng_get :: proc(mng: ^chunk_mng, key: u32) -> ^chunk_point {
    if !pool_alive(&mng.point_pool, key) {
        core.log_error("chunk_mng_get: chunk point [%d] invalid (dead or worse) => stub", key)
        return &mng.point_pool.data_list[0]
    }
    return pool_get(&mng.point_pool, key)
}

chunk_mng_update :: proc(mng: ^chunk_mng) {

}
//
// chunk_mng_query :: proc(mng: ^chunk_mng, rect: [4]f32) -> (queried_points: ^[^]u32, queried_len: ^u32) {
//
// }

//
_chunk_pos_cal :: proc(chunk_size: f32, world_pos: [2]f32) -> [2]i32 {
    return { i32(math.floor(world_pos[0] / chunk_size)), i32(math.floor(world_pos[1] / chunk_size)) }
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
        if !entry.alive || entry.pos == pos { break; }

        idx += 1
        if idx >= mng.cap { idx = 1 }

        if idx == first_idx {
            core.log_error("too much chunks opened => stub")
            return 0, &mng.chunk_map[0]
        }
    }

    ptr := &mng.chunk_map[idx]
    ptr.alive = true
    ptr.pos = pos

    return idx, ptr
}
