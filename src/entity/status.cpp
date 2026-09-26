// the way to temporary mark/store some data to an entity
// - the actual status data stored in a pool, an entity will have the handle (u32 key)
// - a status should have all data for every possible state, handle whatever with all that

module;

#include <cassert>
#include <cstring>

export module entity:status;

import def;
import mem;
import log;

import context;

export namespace sw {

constexpr u32 STATUS_ALIVE_POOL_FLAG = (u32)-1;

// status_pool will be a classic pool instead of sparse-set pool since this is not for iteration
struct status {
    u32 pool_flag;
    u32 pool_key;

    //
    u32 flag;

    u32 owner_entity_key;
    u32 links_in_entity[2];

    // actual data of a status
    u32 stack_count;
    u32 duration_sec;
    u32 last_checked_time_sec;

    f32 damage;
};

struct {
    status *status_pool;
    u32 status_pool_cap;
    u32 status_pool_head_key;
    u32 status_pool_max_key;
    u32 status_pool_len;
} status_sys = {};

// ================================================================================================

void status_sys_init(u32 cap) {
    status_sys.status_pool = (status*)arena_alloc(&omni_arena, cap * sizeof(status));
    status_sys.status_pool_cap = cap;

    // stub
    status_sys.status_pool_head_key = 1;
    status_sys.status_pool_max_key = 1;
    status_sys.status_pool_len = 1;
}

// ================================================================================================

void status_create(u32 flag, u32 *out_key, status **out_ptr) {
    // the out_ptr is supposed to modified afterward, so this just return the bare minimum

    if (status_sys.status_pool_len >= status_sys.status_pool_cap) {
        log_err("status_create: too many statuses (%u) => stub", status_sys.status_pool_len);
        if (out_key) { *out_key = 0; }
        if (out_ptr) { *out_ptr = &status_sys.status_pool[0]; }
        return;
    }

    // get free index from free-list head
    u32 key = status_sys.status_pool_head_key;
    assert(key <= status_sys.status_pool_max_key);

    // update free-list
    if (key == status_sys.status_pool_max_key) {
        status_sys.status_pool_max_key++;
        status_sys.status_pool_head_key++;
    } else {
        status_sys.status_pool_head_key = status_sys.status_pool[key].pool_flag;
    }

    status_sys.status_pool_len++;

    // init status
    status *ptr = &status_sys.status_pool[key];

    memset(ptr, 0, sizeof(status));
    status_sys.status_pool[key].pool_flag = STATUS_ALIVE_POOL_FLAG;
    ptr->pool_key = key;
    ptr->flag = flag;

    if (out_key) { *out_key = key; }
    if (out_ptr) { *out_ptr = ptr; }

    log_trace("created status [%u]: type = %u", key, flag);
}

void status_destroy(u32 key) {
    if (key == 0 || key >= status_sys.status_pool_max_key) {
        log_warn("status_destroy: status [%u] invalid", key);
        return;
    }

    if (status_sys.status_pool[key].pool_flag != STATUS_ALIVE_POOL_FLAG) {
        log_warn("status_destroy: status [%u] already dead", key);
        return;
    }

    // push slot back to free-list head
    status_sys.status_pool[key].pool_flag = status_sys.status_pool_head_key;
    status_sys.status_pool_head_key = key;
    status_sys.status_pool_len--;

    log_trace("destroyed status [%u]", key);
}

status *status_get(u32 key) {
    if (key == 0 || key >= status_sys.status_pool_max_key) {
        log_err("status_get: status [%u] invalid => stub", key);
        return &status_sys.status_pool[0];
    }

    if (status_sys.status_pool[key].pool_flag != STATUS_ALIVE_POOL_FLAG) {
        log_err("status_get: status [%u] is dead => stub", key);
        return &status_sys.status_pool[0];
    }

    return &status_sys.status_pool[key];
}

bool status_alive(u32 key) {
    if (key == 0 || key >= status_sys.status_pool_max_key) {
        return false;
    }
    return status_sys.status_pool[key].pool_flag == STATUS_ALIVE_POOL_FLAG;
}

}
