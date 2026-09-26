// disperse many point in world to chunks, so that can quickly query those
// - world divide into many chunk slot, each chunk slot may store many points, or no point at all
// - this meant to just store points, so that in order to store and query aabbs, this has to required all aabb size < ENTITY_MAX_BOUNDS_SIZE

module;

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

export module entity:chunk;

import def;
import mem;
import log;

import pool;

import context;

export namespace sw {

constexpr f32 CHUNK_MAX_BOUNDS_SIZE = 128.0f;

struct chunk_point {
    u32 pool_key;

    u32 data; // usually the entity/sprite/collider key

    f32 pos[2]; // world pos

    u32 links_in_slot[2];
    i32 chunk_pos[2];
};

struct chunk_slot {
    bool map_alive;

    i32 chunk_pos[2];

    u32 point_list_begin;
    u32 point_list_len;
};

struct chunk_mng {
    f32 chunk_size;

    // all points as pool
    pool<chunk_point> point_pool;

    // all chunk slot as map
    chunk_slot *chunk_slot_map;
    u32 slot_map_cap;
    u32 slot_map_len;
};

// ================================================================================================

// world to chunk pos
void _chunk_pos_cal(chunk_mng *mng, f32 world_pos[2], i32 out_pos[2]);

// add_or_get in map, return true if add, false if get
bool _chunk_map_open(chunk_mng *mng, i32 pos[2], u32 *out_idx, chunk_slot **out_ptr);

// ================================================================================================

void chunk_mng_init(chunk_mng *mng, f32 chunk_size, u32 cap) {
    mng->chunk_size = chunk_size;

    constexpr usize CHUNK_POINT_KEY_FIELD_OFFSET = 0;
    pool_init(&mng->point_pool, cap, CHUNK_POINT_KEY_FIELD_OFFSET);
    mng->chunk_slot_map = (chunk_slot*)arena_alloc(&omni_arena, cap * sizeof(chunk_slot));
    mng->slot_map_cap = cap;

    // stub
    mng->slot_map_len = 1;
}

// ================================================================================================

void chunk_mng_create(chunk_mng *mng, u32 data, f32 center_pos[2], u32 *out_key, chunk_point **out_ptr) {
    // allocate new point instance and insert into matching chunk slot linked-list

    if (mng->slot_map_len >= mng->slot_map_cap) {
        log_err("chunk_mng_create: too many chunk points (%u) => stub", mng->slot_map_len);
        if (out_key) { *out_key = 0; }
        if (out_ptr) { *out_ptr = &mng->point_pool.data_list_ptr[0]; }
        return;
    }

    // create new point entry in pool
    u32 point_key = 0;
    chunk_point *point_ptr = nullptr;
    pool_create(&mng->point_pool, &point_key, &point_ptr);

    point_ptr->data = data;
    point_ptr->pool_key = point_key;
    point_ptr->pos[0] = center_pos[0];
    point_ptr->pos[1] = center_pos[1];

    _chunk_pos_cal(mng, center_pos, point_ptr->chunk_pos);

    // open slot in chunk hash map
    u32 slot_idx = 0;
    chunk_slot *slot_ptr = nullptr;
    _chunk_map_open(mng, point_ptr->chunk_pos, &slot_idx, &slot_ptr);

    // link new point to the head of chunk slot linked-list
    point_ptr->links_in_slot[0] = 0;
    point_ptr->links_in_slot[1] = slot_ptr->point_list_begin;

    if (slot_ptr->point_list_begin != 0) {
        assert(slot_ptr->point_list_begin < mng->point_pool.slot_pool_len);
        chunk_point *begin_point_ptr = pool_get(&mng->point_pool, slot_ptr->point_list_begin);
        assert(begin_point_ptr->chunk_pos[0] == point_ptr->chunk_pos[0] && begin_point_ptr->chunk_pos[1] == point_ptr->chunk_pos[1]);
        begin_point_ptr->links_in_slot[0] = point_key;
    }

    slot_ptr->point_list_begin = point_key;
    slot_ptr->point_list_len++;

    if (out_key) { *out_key = point_key; }
    if (out_ptr) { *out_ptr = point_ptr; }
}

void chunk_mng_destroy(chunk_mng *mng, u32 point_key) {
    // remove point from chunk slot linked-list

    if (!pool_alive(&mng->point_pool, point_key)) {
        log_err("chunk_mng_destroy: instance [%u] invalid (dead or worse)", point_key);
        return;
    }

    chunk_point *point_ptr = pool_get(&mng->point_pool, point_key);

    u32 slot_idx = 0;
    chunk_slot *slot_ptr = nullptr;
    _chunk_map_open(mng, point_ptr->chunk_pos, &slot_idx, &slot_ptr);

    // update previous point's next
    if (point_ptr->links_in_slot[0] != 0) {
        assert(point_ptr->links_in_slot[0] < mng->point_pool.slot_pool_len);
        chunk_point *prev_point_ptr = pool_get(&mng->point_pool, point_ptr->links_in_slot[0]);
        assert(prev_point_ptr->chunk_pos[0] == point_ptr->chunk_pos[0] && prev_point_ptr->chunk_pos[1] == point_ptr->chunk_pos[1]);
        prev_point_ptr->links_in_slot[1] = point_ptr->links_in_slot[1];
    }

    // update next point's previous
    if (point_ptr->links_in_slot[1] != 0) {
        assert(point_ptr->links_in_slot[1] < mng->point_pool.slot_pool_len);
        chunk_point *next_point_ptr = pool_get(&mng->point_pool, point_ptr->links_in_slot[1]);
        assert(next_point_ptr->chunk_pos[0] == point_ptr->chunk_pos[0] && next_point_ptr->chunk_pos[1] == point_ptr->chunk_pos[1]);
        next_point_ptr->links_in_slot[0] = point_ptr->links_in_slot[0];
    }

    // update slot list head
    if (slot_ptr->point_list_begin == point_key) {
        slot_ptr->point_list_begin = point_ptr->links_in_slot[1];
    }

    slot_ptr->point_list_len--;

    pool_destroy(&mng->point_pool, point_key);
}

void chunk_mng_update(chunk_mng *mng, u32 point_key, f32 new_center_pos[2]) {
    // update point world pos to update its chunk pos, chunk slot,...

    if (!pool_alive(&mng->point_pool, point_key)) {
        log_err("chunk_mng_update: instance [%u] invalid (dead or worse)", point_key);
        return;
    }

    chunk_point *point_ptr = pool_get(&mng->point_pool, point_key);

    i32 new_chunk_pos[2];
    _chunk_pos_cal(mng, new_center_pos, new_chunk_pos);

    // point stayed inside the exact same chunk, then no change
    if (point_ptr->chunk_pos[0] == new_chunk_pos[0] && point_ptr->chunk_pos[1] == new_chunk_pos[1]) {
        point_ptr->pos[0] = new_center_pos[0];
        point_ptr->pos[1] = new_center_pos[1];
        return;
    }

    { // remove point from old chunk slot list
        u32 old_slot_idx = 0;
        chunk_slot *old_slot_ptr = nullptr;
        _chunk_map_open(mng, point_ptr->chunk_pos, &old_slot_idx, &old_slot_ptr);

        if (point_ptr->links_in_slot[0] != 0) {
            assert(point_ptr->links_in_slot[0] < mng->point_pool.slot_pool_len);
            chunk_point *prev_point_ptr = pool_get(&mng->point_pool, point_ptr->links_in_slot[0]);
            assert(prev_point_ptr->chunk_pos[0] == point_ptr->chunk_pos[0] && prev_point_ptr->chunk_pos[1] == point_ptr->chunk_pos[1]);
            prev_point_ptr->links_in_slot[1] = point_ptr->links_in_slot[1];
        }

        if (point_ptr->links_in_slot[1] != 0) {
            assert(point_ptr->links_in_slot[1] < mng->point_pool.slot_pool_len);
            chunk_point *next_point_ptr = pool_get(&mng->point_pool, point_ptr->links_in_slot[1]);
            assert(next_point_ptr->chunk_pos[0] == point_ptr->chunk_pos[0] && next_point_ptr->chunk_pos[1] == point_ptr->chunk_pos[1]);
            next_point_ptr->links_in_slot[0] = point_ptr->links_in_slot[0];
        }

        if (old_slot_ptr->point_list_begin == point_key) {
            old_slot_ptr->point_list_begin = point_ptr->links_in_slot[1];
        }

        old_slot_ptr->point_list_len--;
    }

    // clean to re-insert
    point_ptr->pos[0] = new_center_pos[0];
    point_ptr->pos[1] = new_center_pos[1];
    point_ptr->chunk_pos[0] = new_chunk_pos[0];
    point_ptr->chunk_pos[1] = new_chunk_pos[1];

    { // insert point into new chunk slot list
        u32 new_slot_idx = 0;
        chunk_slot *new_slot_ptr = nullptr;
        _chunk_map_open(mng, point_ptr->chunk_pos, &new_slot_idx, &new_slot_ptr);

        point_ptr->links_in_slot[0] = 0;
        point_ptr->links_in_slot[1] = new_slot_ptr->point_list_begin;

        if (new_slot_ptr->point_list_begin != 0) {
            assert(new_slot_ptr->point_list_begin < mng->point_pool.slot_pool_len);
            chunk_point *begin_point_ptr = pool_get(&mng->point_pool, new_slot_ptr->point_list_begin);
            assert(begin_point_ptr->chunk_pos[0] == point_ptr->chunk_pos[0] && begin_point_ptr->chunk_pos[1] == point_ptr->chunk_pos[1]);
            begin_point_ptr->links_in_slot[0] = point_key;
        }

        new_slot_ptr->point_list_begin = point_key;
        new_slot_ptr->point_list_len++;
    }
}

chunk_point *chunk_mng_get(chunk_mng *mng, u32 point_key) {
    if (!pool_alive(&mng->point_pool, point_key)) {
        log_err("chunk_mng_get: instance [%u] invalid (dead or worse) => stub", point_key);
        return &mng->point_pool.data_list_ptr[0];
    }
    return pool_get(&mng->point_pool, point_key);
}

void chunk_mng_query(chunk_mng *mng, f32 rect[4], u32 **out_list, u32 *out_list_len, arena *target_arena = tick_arena_ptr) {
    if (!out_list) { return; } // why

    u32 *queried_list = nullptr;
    u32 queried_len = 0;
    u32 queried_cap = 0;

    // pad all query with MAX_BOUNDS_SIZE
    f32 bounds_padding = CHUNK_MAX_BOUNDS_SIZE / 2.0f;
    f32 min_world_pos[2] = { rect[0] - bounds_padding, rect[1] - bounds_padding };
    f32 max_world_pos[2] = { rect[0] + rect[2] + bounds_padding, rect[1] + rect[3] + bounds_padding };

    // scanned chunks
    i32 min_chunk_pos[2];
    i32 max_chunk_pos[2];
    _chunk_pos_cal(mng, min_world_pos, min_chunk_pos);
    _chunk_pos_cal(mng, max_world_pos, max_chunk_pos);

    // scan covered chunk coordinates
    for (i32 chunk_y = min_chunk_pos[1]; chunk_y <= max_chunk_pos[1]; chunk_y++) {
        for (i32 chunk_x = min_chunk_pos[0]; chunk_x <= max_chunk_pos[0]; chunk_x++) {
            i32 target_chunk_pos[2] = { chunk_x, chunk_y };
            u32 slot_idx = 0;
            chunk_slot *slot_ptr = nullptr;

            if (! _chunk_map_open(mng, target_chunk_pos, &slot_idx, &slot_ptr)) { continue; }

            // iterate through points stored in current chunk slot
            u32 point_key = slot_ptr->point_list_begin;
            while (point_key != 0) {
                chunk_point *point_ptr = pool_get(&mng->point_pool, point_key);

                // test point against query range
                if (point_ptr->pos[0] >= min_world_pos[0] && point_ptr->pos[0] <= max_world_pos[0] &&
                    point_ptr->pos[1] >= min_world_pos[1] && point_ptr->pos[1] <= max_world_pos[1]) {
                    // push into queried_list
                    if (queried_len >= queried_cap) {
                        queried_cap = (queried_cap < 2) ? 2 : (queried_cap * 4);
                        u32 *new_buffer_ptr = (u32*)arena_alloc_raw(target_arena, queried_cap * sizeof(u32));
                        if (queried_list) {
                            memcpy(new_buffer_ptr, queried_list, queried_len * sizeof(u32));
                        }
                        queried_list = new_buffer_ptr;
                    }

                    queried_list[queried_len] = point_key;
                    queried_len++;
                }

                point_key = point_ptr->links_in_slot[1];
            }
        }
    }

    if (out_list) { *out_list = queried_list; }
    if (out_list_len) { *out_list_len = queried_len; }
}

// ================================================================================================

bool _chunk_mng_validate(chunk_mng *mng) {
    u32 total_active_instances = 0;
    u32 iter_idx = 0;
    u32 point_key = 0;
    chunk_point *point_ptr = nullptr;

    // validate all active points and bidirectional list links
    while (pool_iterate(&mng->point_pool, &iter_idx, &point_key, &point_ptr)) {
        total_active_instances++;

        i32 expected_chunk_pos[2];
        _chunk_pos_cal(mng, point_ptr->pos, expected_chunk_pos);
        if (point_ptr->chunk_pos[0] != expected_chunk_pos[0] || point_ptr->chunk_pos[1] != expected_chunk_pos[1]) {
            log_trace("chunk_mng validate: instance [%u] pos mismatch: stored [%d %d] vs computed [%d %d]",
                      point_key, point_ptr->chunk_pos[0], point_ptr->chunk_pos[1], expected_chunk_pos[0], expected_chunk_pos[1]);
            return false;
        }

        // prev link
        u32 prev_point_key = point_ptr->links_in_slot[0];
        if (prev_point_key != 0) {
            if (!pool_alive(&mng->point_pool, prev_point_key)) {
                log_trace("chunk_mng validate: instance [%u] prev link points to dead key [%u]", point_key, prev_point_key);
                return false;
            }
            chunk_point *prev_point_ptr = pool_get(&mng->point_pool, prev_point_key);
            if (prev_point_ptr->links_in_slot[1] != point_key) {
                log_trace("chunk_mng validate: link break: [%u].links[0] = [%u], but [%u].links[1] = [%u]",
                          point_key, prev_point_key, prev_point_key, prev_point_ptr->links_in_slot[1]);
                return false;
            }
            if (prev_point_ptr->chunk_pos[0] != point_ptr->chunk_pos[0] || prev_point_ptr->chunk_pos[1] != point_ptr->chunk_pos[1]) {
                log_trace("chunk_mng validate: adjacent instances [%u] and [%u] have different chunk positions", point_key, prev_point_key);
                return false;
            }
        }

        // next link
        u32 next_point_key = point_ptr->links_in_slot[1];
        if (next_point_key != 0) {
            if (!pool_alive(&mng->point_pool, next_point_key)) {
                log_trace("chunk_mng validate: instance [%u] next link points to dead key [%u]", point_key, next_point_key);
                return false;
            }
            chunk_point *next_point_ptr = pool_get(&mng->point_pool, next_point_key);
            if (next_point_ptr->links_in_slot[0] != point_key) {
                log_trace("chunk_mng validate: link break: [%u].links[1] = [%u], but [%u].links[0] = [%u]",
                          point_key, next_point_key, next_point_key, next_point_ptr->links_in_slot[0]);
                return false;
            }
            if (next_point_ptr->chunk_pos[0] != point_ptr->chunk_pos[0] || next_point_ptr->chunk_pos[1] != point_ptr->chunk_pos[1]) {
                log_trace("chunk_mng validate: adjacent instances [%u] and [%u] have different chunk positions", point_key, next_point_key);
                return false;
            }
        }
    }

    u32 total_chunk_instances = 0;
    bool *visited_slots = (bool*)calloc(mng->point_pool.slot_pool_len, sizeof(bool));
    if (!visited_slots) {
        return false;
    }

    // traverse all chunk map slots and confirm membership
    for (u32 slot_idx = 1; slot_idx < mng->slot_map_cap; slot_idx++) {
        chunk_slot *slot_entry_ptr = &mng->chunk_slot_map[slot_idx];
        if (!slot_entry_ptr->map_alive) {
            continue;
        }

        u32 current_point_key = slot_entry_ptr->point_list_begin;
        u32 slot_points_counted = 0;

        if (current_point_key != 0) {
            chunk_point *head_point_ptr = pool_get(&mng->point_pool, current_point_key);
            if (head_point_ptr->links_in_slot[0] != 0) {
                log_trace("chunk_mng validate: chunk [%d %d] head [%u] has non-zero prev link (%u)",
                          slot_entry_ptr->chunk_pos[0], slot_entry_ptr->chunk_pos[1], current_point_key, head_point_ptr->links_in_slot[0]);
                free(visited_slots);
                return false;
            }
        }

        while (current_point_key != 0) {
            if (!pool_alive(&mng->point_pool, current_point_key)) {
                log_trace("chunk_mng validate: chunk [%d %d] list contains dead instance [%u]",
                          slot_entry_ptr->chunk_pos[0], slot_entry_ptr->chunk_pos[1], current_point_key);
                free(visited_slots);
                return false;
            }

            if (visited_slots[current_point_key]) {
                log_trace("chunk_mng validate: cycle detected in chunk [%d %d] list at instance [%u]",
                          slot_entry_ptr->chunk_pos[0], slot_entry_ptr->chunk_pos[1], current_point_key);
                free(visited_slots);
                return false;
            }
            visited_slots[current_point_key] = true;

            chunk_point *current_point_ptr = pool_get(&mng->point_pool, current_point_key);
            if (current_point_ptr->chunk_pos[0] != slot_entry_ptr->chunk_pos[0] || current_point_ptr->chunk_pos[1] != slot_entry_ptr->chunk_pos[1]) {
                log_trace("chunk_mng validate: instance [%u] chunk_pos [%d %d] does not match chunk [%d %d]",
                          current_point_key, current_point_ptr->chunk_pos[0], current_point_ptr->chunk_pos[1], slot_entry_ptr->chunk_pos[0], slot_entry_ptr->chunk_pos[1]);
                free(visited_slots);
                return false;
            }

            slot_points_counted++;
            current_point_key = current_point_ptr->links_in_slot[1];
        }

        if (slot_points_counted != slot_entry_ptr->point_list_len) {
            log_trace("chunk_mng validate: chunk [%d %d] counted %u instances but instance_len = %u",
                      slot_entry_ptr->chunk_pos[0], slot_entry_ptr->chunk_pos[1], slot_points_counted, slot_entry_ptr->point_list_len);
            free(visited_slots);
            return false;
        }

        total_chunk_instances += slot_points_counted;
    }

    free(visited_slots);

    if (total_chunk_instances != total_active_instances) {
        log_trace("chunk_mng validate: instances across chunks (%u) != pool active count (%u)",
                  total_chunk_instances, total_active_instances);
        return false;
    }

    return true;
}

void _chunk_mng_debug_log(chunk_mng *mng) {
    usize buffer_size = 1024 + (usize)mng->slot_map_cap * 128;
    char *debug_buf = (char*)malloc(buffer_size);
    if (!debug_buf) {
        return;
    }

    usize string_offset = 0;
    string_offset += snprintf(debug_buf + string_offset, buffer_size - string_offset,
                              "--- CHUNK MNG DEBUG: len: %u; cap: %u; pool_len: %u; pool_cap: %u ---\nSTATUS: %s\n",
                              mng->slot_map_len, mng->slot_map_cap, mng->point_pool.data_list_len, mng->point_pool.cap,
                              _chunk_mng_validate(mng) ? "ok" : "INVALID*");

    for (u32 slot_idx = 1; slot_idx < mng->slot_map_cap; slot_idx++) {
        chunk_slot *slot_entry_ptr = &mng->chunk_slot_map[slot_idx];
        if (!slot_entry_ptr->map_alive) {
            continue;
        }

        string_offset += snprintf(debug_buf + string_offset, buffer_size - string_offset, "chunk [%d %d] has %u instances:\n",
                                  slot_entry_ptr->chunk_pos[0], slot_entry_ptr->chunk_pos[1], slot_entry_ptr->point_list_len);

        u32 current_point_key = slot_entry_ptr->point_list_begin;
        while (current_point_key != 0) {
            if (!pool_alive(&mng->point_pool, current_point_key)) {
                string_offset += snprintf(debug_buf + string_offset, buffer_size - string_offset, "- instance [%u]: <DEAD/INVALID>\n", current_point_key);
                break;
            }

            chunk_point *point_ptr = pool_get(&mng->point_pool, current_point_key);
            string_offset += snprintf(debug_buf + string_offset, buffer_size - string_offset,
                                      "- instance [%u]: data = [%u]; center = (%.1f, %.1f) in chunk [%d %d]\n",
                                      current_point_key, point_ptr->data, point_ptr->pos[0], point_ptr->pos[1], point_ptr->chunk_pos[0], point_ptr->chunk_pos[1]);

            current_point_key = point_ptr->links_in_slot[1];
        }
    }

    log_info("%s", debug_buf);
    free(debug_buf);
}

// ================================================================================================

f32* chunk_cal_center_rect(f32 rect[4], f32 out_center_pos[2]) {
    out_center_pos[0] = rect[0] + rect[2] / 2.0f;
    out_center_pos[1] = rect[1] + rect[3] / 2.0f;
    return out_center_pos;
}

// ================================================================================================

void _chunk_pos_cal(chunk_mng *mng, f32 world_pos[2], i32 out_pos[2]) {
    out_pos[0] = (i32)floorf(world_pos[0] / mng->chunk_size);
    out_pos[1] = (i32)floorf(world_pos[1] / mng->chunk_size);
}

bool _chunk_map_open(chunk_mng *mng, i32 pos[2], u32 *out_idx, chunk_slot **out_ptr) {
    // splitmix64-style hash on 64-bit combined grid coordinates
    u64 coord_x = (u64)(u32)pos[0];
    u64 coord_y = (u64)(u32)pos[1];
    u64 hash_key = (coord_x << 32) | coord_y;

    hash_key ^= hash_key >> 30;
    hash_key *= 0xbf58476d1ce4e5b9ULL;
    hash_key ^= hash_key >> 27;
    hash_key *= 0x94d049bb133111ebULL;
    hash_key ^= hash_key >> 31;

    u32 first_slot_idx = (u32)(hash_key % (u64)(mng->slot_map_cap - 1)) + 1;
    u32 probe_slot_idx = first_slot_idx;

    bool is_created_new_slot = false;

    // open addressing with linear probing
    while (true) {
        chunk_slot *slot_entry_ptr = &mng->chunk_slot_map[probe_slot_idx];
        if (!slot_entry_ptr->map_alive) {
            slot_entry_ptr->map_alive = true;
            slot_entry_ptr->chunk_pos[0] = pos[0];
            slot_entry_ptr->chunk_pos[1] = pos[1];
            is_created_new_slot = true;
            break;
        }

        if (slot_entry_ptr->chunk_pos[0] == pos[0] && slot_entry_ptr->chunk_pos[1] == pos[1]) {
            break;
        }

        probe_slot_idx++;
        if (probe_slot_idx >= mng->slot_map_cap) {
            probe_slot_idx = 1;
        }

        // cycled through table without finding an empty slot
        if (probe_slot_idx == first_slot_idx) {
            log_err("too much chunks opened (%u) => stub", mng->slot_map_cap);
            if (out_idx) { *out_idx = 0; }
            if (out_ptr) { *out_ptr = &mng->chunk_slot_map[0]; }
            return false;
        }
    }

    if (out_idx) { *out_idx = probe_slot_idx; }
    if (out_ptr) { *out_ptr = &mng->chunk_slot_map[probe_slot_idx]; }

    return is_created_new_slot;
}

}
