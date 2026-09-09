#pragma once

#include "define.h"

typedef struct {
    u32 pool_flag;

    f32 x, y; // become offset to entity of entity_idx != 0
    f32 w, h;

    u8 flag;

    u32 tree_node_idx;

    // for entity stuff
    u32 entity_idx;
    u32 next;
} collider;

typedef struct {
    u32 pool_flag;

    u32 collider_idx;

    u32 parent;
    u32 child[2];
    u32 height;

    f32 x, y, w, h;

    u8 flag;
} collider_node;

struct collider_sys {
    // collider pool
    collider *collider_pool;
    usize collider_cap;
    usize collider_head;
    usize collider_max_idx;
    u32 collider_len;

    // tree
    collider_node *tree_pool;
    usize tree_cap;
    usize tree_head;
    usize tree_max_idx;
    u32 tree_len;

    u32 tree_root;

    f32 fat_aabb_offset;
};
extern struct collider_sys collider_sys;

// =============================================================================
void collider_sys_init(usize cap, f32 fat_aabb_offset);

// =============================================================================
u32 collider_create(collider **r_collider);
void collider_destroy(u32 idx);

[[nodiscard]] collider *collider_get(u32 idx);

void collider_add_to_tree(u32 idx);

// =============================================================================
void collider_sys_update();

void collider_sys_draw_debug();
