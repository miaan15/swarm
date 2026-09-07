#pragma once

#include "define.h"

typedef struct {
    u32 tex;
    f32 x, y, w, h;
} sprite_asset;

typedef struct {
    u32 pool_flag;

    u32 asset_idx;
    f32 x, y;
    i32 z;

    // for entity stuff
    u32 entity_idx;
    u32 next;
} sprite;

struct sprite_sys {
    sprite_asset *asset_arr;
    usize asset_cap;
    usize asset_len;

    sprite *sprite_pool;
    usize cap;
    usize head;
    usize max_idx;
    u32 len;
};

extern struct sprite_sys sprite_sys;

void sprite_sys_init(usize asset_cap, usize sprite_cap);

u32 sprite_asset_make(u32 texture_id, u32 w, u32 h);
sprite_asset sprite_asset_get(usize idx);

u32 sprite_create(u32 asset_idx);
void sprite_destroy(u32 idx);

sprite *sprite_get(usize idx);
