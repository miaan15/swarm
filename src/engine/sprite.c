#include "sprite.h"

#include "context.h"
#include "log.h"

#define ALIVE_POOL_FLAG ((u32)-1)

struct sprite_sys sprite_sys = {0};

void sprite_sys_init(usize asset_cap, usize sprite_cap) {
    sprite_sys.asset_arr = arena_alloc(&omni_arena, asset_cap * sizeof(sprite_asset));
    sprite_sys.asset_cap = asset_cap;

    sprite_sys.sprite_pool = arena_alloc(&omni_arena, sprite_cap * sizeof(sprite));
    sprite_sys.cap = sprite_cap;

    // stub
    sprite_sys.asset_len = 1;
    // TODO assign stub sprite asset

    sprite_sys.head = sprite_sys.max_idx = sprite_sys.len = 1;
    // TODO assign stub sprite
}

u32 sprite_asset_make(u32 tex, u32 x, u32 y, u32 w, u32 h) {
    sprite_sys.asset_arr[sprite_sys.asset_len] = (sprite_asset){tex, x, y, w, h};
    return sprite_sys.asset_len++;
}

sprite_asset sprite_asset_get(usize idx) {
    return sprite_sys.asset_arr[idx];
}

u32 sprite_create(u32 asset_idx) {
    if (sprite_sys.len >= sprite_sys.cap) {
        log_err("sprite_create(): too much sprites => stub");
        return 0;
    }

    usize idx = sprite_sys.head;
    sprite *spr = &sprite_sys.sprite_pool[idx];

    if (idx == sprite_sys.max_idx) {
        ++sprite_sys.max_idx;
        ++sprite_sys.head;
    } else {
        sprite_sys.head = spr->pool_flag;
    }

    ++sprite_sys.len;

    memset(spr, 0, sizeof(sprite));
    spr->asset_idx = asset_idx;

    log_debug("Created Sprite [%u]", idx);

    return idx;
}

void sprite_destroy(u32 idx) {
    sprite *spr = &sprite_sys.sprite_pool[idx];

    if (spr->pool_flag != ALIVE_POOL_FLAG) {
        log_warn("sprite_destroy(): sprite already dead");
        return;
    }

    spr->pool_flag = sprite_sys.head;
    sprite_sys.head = idx;

    --sprite_sys.len;

    log_debug("Destroyed Sprite [%u]", idx);
}

sprite *sprite_get(usize idx) {
    return &sprite_sys.sprite_pool[idx];
}
