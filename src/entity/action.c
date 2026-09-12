#include "action.h"

#include "context.h"
#include "log.h"
#include <assert.h>

struct action_sys action_sys = {0};

// =============================================================================
void action_sys_init(usize cap) {
    action_sys.action_buffer = arena_alloc(&omni_arena, cap * sizeof(action));
    action_sys.cap = cap;
    action_sys.len = 0;
}

// =============================================================================
void action_make(u32 from, u32 to, u32 type, u32 *data_3) {
    if (action_sys.len >= action_sys.cap) {
        log_err("action_make(): too much actions");
        assert(false);
        return;
    }

    action *act = &action_sys.action_buffer[action_sys.len++];
    act->from = from;
    act->to = to;
    act->type = type;
    memcpy(act->data, data_3, 3 * sizeof(f32));

    log_debug("Made action: from [%u] to [%u] of type [%u]", from, to, type);
}

// =============================================================================
void action_sys_update() {
    for (usize i = 0; i < action_sys.len; ++i) {
        action* act = &action_sys.action_buffer[i];

        fn_handle_action(act);
    }

    action_sys.len = 0;
}
