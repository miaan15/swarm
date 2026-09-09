#pragma once

#include "define.h"

typedef struct {
    u32 pool_flag;

    f32 x, y; // become offset to entity of entity_idx != 0
    f32 w, h;

    u8 flag;

    u32 tree_node;

    // for entity stuff
    u32 entity_idx;
    u32 next;
} box;

typedef struct {
    u32 pool_flag;

    u32 box_idx;
    
    u32 parent;
    u32 child[2];
    u32 height;

    f32 x, y, w, h;

    u8 flag;
} box_node;

#define FAT_BOX_OFFSET 5
struct box_sys {
    // box pool
    box *box_pool;
    usize box_cap;
    usize box_head;
    usize box_max_idx;
    u32 box_len;

    // tree
    box_node *tree_pool;
    usize tree_cap;
    usize tree_head;
    usize tree_max_idx;
    u32 tree_len;

    u32 tree_root;
};
extern struct box_sys box_sys;

// =============================================================================
void box_sys_init(usize cap);

// =============================================================================
u32 box_create(box **r_box);
void box_destroy(u32 idx);

[[nodiscard]] box *box_get(u32 idx);

void box_add_tree(u32 idx);

// =============================================================================
void box_sys_update();

void box_sys_draw_debug();
