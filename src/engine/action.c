#include "action.h"

#include "context.h"
#include "log.h"
#include "proxy.h"
#include <inttypes.h>

struct action_sys action_sys = {0};

// =============================================================================
void action_sys_init(usize cap) {
    action_sys.action_buffer = arena_alloc(&omni_arena, cap * sizeof(action));
    action_sys.cap = cap;
    action_sys.len = 0;
}

// =============================================================================
void action_make(u32 from, u32 to, u32 type, u64 data) {
    if (action_sys.len >= action_sys.cap) {
        log_err("action_make(): too much actions");
        return;
    }

    action_sys.action_buffer[action_sys.len++] = (action){ from, to, type, data };

    log_debug("Made action: from [%u] to [%u] of type [%u]: 0x%016" PRIx64, from, to, type, data);
}

// =============================================================================
void action_sys_update() {
    for (usize i = 0; i < action_sys.len; ++i) {
        action act = action_sys.action_buffer[i];

        action_handle_fn(act);
    }

    action_sys.len = 0;
}
