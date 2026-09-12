#pragma once

#include "define.h"

typedef struct {
    u32 from, to;
    u32 type;
    u32 data[3];
} action;

struct action_sys {
    action *action_buffer;
    usize cap;
    usize len;
};
extern struct action_sys action_sys;

// =============================================================================
void action_sys_init(usize cap);

// =============================================================================
void action_make(u32 from, u32 to, u32 type, u32 *data_3);

// =============================================================================
void action_sys_update();
