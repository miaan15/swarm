#pragma once

#include "define.h"

constexpr u32 CHUNK_SIZE = 512;

typedef struct {
    bool alive;
    i32 cx, cy;

    u32 entity_begin;
    u32 sprite_begin;
    u32 collider_begin;
} chunk_entry;

struct chunk_sys {
    chunk_entry *chunk_map;
    usize chunk_cap;
    u32 chunk_len;
};
extern struct chunk_sys chunk_sys;

// =============================================================================
void chunk_sys_init(usize cap);

// =============================================================================
void chunk_cal_pos(f32 x, f32 y, i32 *cx, i32 *cy);

void chunk_add_entity(u32 ett_idx);
void chunk_add_sprite(u32 spr_idx);
void chunk_add_collider(u32 col_idx);

void chunk_remv_entity(u32 ett_idx);
void chunk_remv_sprite(u32 spr_idx);
void chunk_remv_collider(u32 col_idx);

void chunk_update_entity(u32 ett_idx);
void chunk_update_sprite(u32 spr_idx);
void chunk_update_collider(u32 col_idx);

void chunk_query_entity(f32 x, f32 y, f32 w, f32 h, u32 **ett_list, usize *ett_list_len);
void chunk_query_sprite(f32 x, f32 y, f32 w, f32 h, u32 **spr_list, usize *spr_list_len);
void chunk_query_collider(f32 x, f32 y, f32 w, f32 h, u32 **col_list, usize *col_list_len);
void chunk_query(f32 x, f32 y, f32 w, f32 h, chunk_entry **entry_list, usize *entry_list_len);
