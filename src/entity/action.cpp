// the main, general way for 2 entities to send some data to each other
// - actions only exist and process in a single frame
// - action is designed to be like immediate draw call, so no delete or access back an action
// - an action only carry 4 u32 (1 for type, 3 for other), everything need to fit into that

export module entity:action;

import def;
import mem;
import log;

import context;

export namespace sw {

struct action {
    u32 from;
    u32 to;
    u32 tag;
    u32 data[3];
};

// action_sys stores all the actions in a single tick
struct {
    action *buffer_ptr;
    u32 buffer_cap;
    u32 buffer_len;
} action_sys = {};

void action_sys_init(u32 cap) {
    action_sys.buffer_ptr = (action*)arena_alloc(&omni_arena, cap * sizeof(action));
    action_sys.buffer_cap = cap;
    action_sys.buffer_len = 0;
}

void action_make(u32 from, u32 to, u32 tag, const u32 data[3]) {
    if (action_sys.buffer_len >= action_sys.buffer_cap) {
        log_err("action_make: too much actions (%u)", action_sys.buffer_len);
        return;
    }

    action_sys.buffer_ptr[action_sys.buffer_len] = action{
        from, to, tag,
        { data[0], data[1], data[2] }
    };
    action_sys.buffer_len++;

    log_trace("make action: from [%u] to [%u]; tag = [%u]; data = (%u %u %u)", from, to, tag, data[0], data[1], data[2]);
}

void action_sys_update() {
    for (u32 i = 0; i < action_sys.buffer_len; ++i) {
        // TODO
    }

    // reset buffer
    action_sys.buffer_len = 0;
}

}
