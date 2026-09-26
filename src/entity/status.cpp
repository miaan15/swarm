// the way to temporary mark/store some data to an entity
// - the actual status data stored in a pool, an entity will have the handle (u32 key)
// - a status should have all data for every possible state, handle whatever with all that

module;

#include <cassert>
#include <cstddef>

export module entity:status;

import def;
import mem;
import log;
import context;
import pool_simple;

export namespace sw {

struct status {
    u32 pool_key;

    //
    u32 flag;

    u32 owner_entity_key;
    u32 links_in_entity_list[2];

    // actual data of a status
    u32 stack_count;
    u32 duration_sec;
    u32 last_checked_time_sec;

    f32 damage;
};

struct {
    pool_simple<status> pool;
} status_sys = {};

// ================================================================================================

void status_sys_init(u32 cap) {
    pool_simple_init(&status_sys.pool, cap, offsetof(status, pool_key));
}

// ================================================================================================

void status_create(u32 flag, u32 *out_status_key, status **out_status_ptr) {
    // the out_ptr is supposed to modified afterward, so this just return the bare minimum

    u32 status_key = 0;
    status *status_ptr = nullptr;
    pool_simple_create(&status_sys.pool, &status_key, &status_ptr);

    if (status_key != 0) {
        status_ptr->flag = flag;
        log_trace("created status [%u]: type = %u", status_key, flag);
    }

    if (out_status_key) { *out_status_key = status_key; }
    if (out_status_ptr) { *out_status_ptr = status_ptr; }
}

void status_destroy(u32 status_key) {
    if (pool_simple_destroy(&status_sys.pool, status_key)) {
        log_trace("destroyed status [%u]", status_key);
    }
}

status *status_get(u32 status_key) {
    return pool_simple_get(&status_sys.pool, status_key);
}

bool status_alive(u32 status_key) {
    return pool_simple_alive(&status_sys.pool, status_key);
}

}
