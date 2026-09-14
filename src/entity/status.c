#include "status.h"

#include "context.h"
#include "log.h"
#include <assert.h>

struct status_sys status_sys = {0};

// =============================================================================
void status_sys_init(usize cap) {
    status_sys.status_pool = arena_alloc(&omni_arena, cap * sizeof(status));
    status_sys.status_cap = cap;

    // stub
    status_sys.status_head = status_sys.status_max_idx = status_sys.status_len = 1;
}

// =============================================================================
u32 status_create(u32 type, status **r_status) {
    if (status_sys.status_len >= status_sys.status_cap) {
        log_err("status_create(): too much statuses => stub");
        assert(false);
        return 0;
    }

    usize idx = status_sys.status_head;
    status *stt = &status_sys.status_pool[idx];

    if (idx == status_sys.status_max_idx) {
        ++status_sys.status_max_idx;
        ++status_sys.status_head;
    } else {
        status_sys.status_head = stt->pool_flag;
    }

    ++status_sys.status_len;

    // setup status
    memset(stt, 0, sizeof(status));
    stt->pool_flag = ALIVE_POOL_FLAG;
    stt->pool_idx = idx;

    stt->type = type;

    log_debug("Created Status [%u]: type = [%u]", idx, type);

    if (r_status != nullptr) *r_status = stt;
    return idx;
}

void status_destroy(u32 idx) {
    status *stt = &status_sys.status_pool[idx];

    if (stt->pool_flag != ALIVE_POOL_FLAG) {
        log_warn("status_destroy(): status already dead");
        return;
    }

    stt->pool_flag = status_sys.status_head;
    status_sys.status_head = idx;

    --status_sys.status_len;

    log_debug("Destroyed Status [%u]", idx);
}

[[nodiscard]] status *status_get(u32 idx) {
    if (idx == 0 || idx >= status_sys.status_max_idx) {
        log_err("status_get(): status invalid => stub");
        assert(false);
        return &status_sys.status_pool[idx];
    }
    return &status_sys.status_pool[idx];
}

// =============================================================================
void status_sys_update() {
    for (usize i = 1; i < status_sys.status_max_idx; ++i) {
        status *stt = status_get(i);
        if (stt->pool_flag != ALIVE_POOL_FLAG) continue;

        fn_handle_status(stt);
    }
}
