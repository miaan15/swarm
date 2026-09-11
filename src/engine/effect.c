#include "effect.h"

#include "context.h"
#include "log.h"
#include "proxy.h"

struct effect_sys effect_sys = {0};

// =============================================================================
void effect_sys_init(usize cap) {
    effect_sys.effect_pool = arena_alloc(&omni_arena, cap * sizeof(effect));
    effect_sys.cap = cap;

    // stub
    effect_sys.head = effect_sys.max_idx = effect_sys.len = 1;
}

// =============================================================================
u32 effect_create(u32 type, effect **r_effect) {
    if (effect_sys.len >= effect_sys.cap) {
        log_err("effect_create(): too much effects => stub");
        return 0;
    }

    usize idx = effect_sys.head;
    effect *eff = &effect_sys.effect_pool[idx];

    if (idx == effect_sys.max_idx) {
        ++effect_sys.max_idx;
        ++effect_sys.head;
    } else {
        effect_sys.head = eff->pool_flag;
    }

    ++effect_sys.len;

    // setup effect
    memset(eff, 0, sizeof(effect));
    eff->pool_flag = ALIVE_POOL_FLAG;
    eff->pool_idx = idx;

    eff->type = type;

    log_debug("Created Effect [%u]: type = [%u]", idx, type);

    if (r_effect != nullptr) *r_effect = eff;
    return idx;
}

void effect_destroy(u32 idx) {
    effect *eff = &effect_sys.effect_pool[idx];

    if (eff->pool_flag != ALIVE_POOL_FLAG) {
        log_warn("effect_destroy(): effect already dead");
        return;
    }

    eff->pool_flag = effect_sys.head;
    effect_sys.head = idx;

    --effect_sys.len;

    log_debug("Destroyed Effect [%u]", idx);
}

[[nodiscard]] effect *effect_get(usize idx) {
    if (idx == 0 || idx >= effect_sys.max_idx) {
        log_err("effect_get(): effect invalid => stub");
        return &effect_sys.effect_pool[idx];
    }
    return &effect_sys.effect_pool[idx];
}

// =============================================================================
void effect_sys_update() {
    for (usize i = 1; i < effect_sys.max_idx; ++i) {
        effect *eff = effect_get(i);
        if (eff->pool_flag != ALIVE_POOL_FLAG) continue;

        effect_handle_fn(eff);
    }
}
