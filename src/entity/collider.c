#include "collider.h"

#include "context.h"
#include "log.h"
#include <assert.h>
#include <raylib.h>

struct collider_sys collider_sys = {0};

// =============================================================================
void collider_sys_init(usize cap) {
    // collider pool
    collider_sys.collider_pool = arena_alloc(&omni_arena, cap * sizeof(collider));
    collider_sys.collider_cap = cap;

    // stub
    collider_sys.collider_head = collider_sys.collider_max_idx = collider_sys.collider_len = 1;
}

// =============================================================================
u32 collider_create(f32 w, f32 h, collider **r_collider) {
    if (collider_sys.collider_len >= collider_sys.collider_cap) {
        log_err("collider_create(): too much collideress => stub");
        assert(false);
        return 0;
    }

    usize idx = collider_sys.collider_head;
    collider *col = &collider_sys.collider_pool[idx];

    if (idx == collider_sys.collider_max_idx) {
        ++collider_sys.collider_max_idx;
        ++collider_sys.collider_head;
    } else {
        collider_sys.collider_head = col->pool_flag;
    }

    ++collider_sys.collider_len;

    // setup collider
    memset(col, 0, sizeof(collider));
    col->pool_flag = ALIVE_POOL_FLAG;
    col->idx = idx;

    col->src_w = w;
    col->src_h = h;

    chunk_add_collider(idx);

    log_debug("Created Collider [%u]", idx);

    if (r_collider != nullptr) *r_collider = col;
    return idx;
}

void collider_destroy(u32 idx) {
    collider *col = &collider_sys.collider_pool[idx];

    if (col->pool_flag != ALIVE_POOL_FLAG) {
        log_warn("collider_destroy(): collider already dead");
        return;
    }

    col->pool_flag = collider_sys.collider_head;
    collider_sys.collider_head = idx;

    --collider_sys.collider_len;

    chunk_remv_collider(idx);

    log_debug("Destroyed Collider [%u]", idx);
}

[[nodiscard]] collider *collider_get(u32 idx) {
    if (idx == 0 || idx >= collider_sys.collider_max_idx) {
        log_err("collider_get(): collider invalid => stub");
        assert(false);
        return &collider_sys.collider_pool[idx];
    }
    return &collider_sys.collider_pool[idx];
}

// =============================================================================
void collider_sys_update() {
    for (usize i = 1; i < collider_sys.collider_max_idx; ++i) {
        collider *col = collider_get(i);
        if (col->pool_flag != ALIVE_POOL_FLAG) continue;

        chunk_update_collider(i);
    }
}
