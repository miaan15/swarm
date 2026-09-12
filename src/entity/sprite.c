#include "sprite.h"

#include "context.h"
#include "draw.h"
#include "log.h"
#include <assert.h>

struct sprite_sys sprite_sys = {0};

// =============================================================================
void sprite_sys_init(usize profile_cap, usize sprite_cap) {
    // profiles
    sprite_sys.profile_arr = arena_alloc(&omni_arena, profile_cap * sizeof(sprite_profile));
    sprite_sys.profile_cap = profile_cap;

    // sprites
    sprite_sys.sprite_pool = arena_alloc(&omni_arena, sprite_cap * sizeof(sprite));
    sprite_sys.cap = sprite_cap;

    // stub
    sprite_sys.profile_len = 1;
    sprite_sys.profile_arr[0] = (sprite_profile){0};
    sprite_sys.profile_arr[0].w = 16;
    sprite_sys.profile_arr[0].h = 16;

    sprite_sys.head = sprite_sys.max_idx = sprite_sys.len = 1;
    sprite_sys.sprite_pool[0] = (sprite){0};
    sprite_sys.sprite_pool[0].sw = 16;
    sprite_sys.sprite_pool[0].sh = 16;
}

// =============================================================================
u32 sprite_profile_create(u32 tex, f32 x, f32 y, f32 w, f32 h) {
    if (tex == 0 || tex >= draw_sys.tex_len) {
        log_err("sprite_profile_create(): Texture [%u] is invalid => tex = stub", tex);
        assert(false);
        tex = 0;
    }

    sprite_sys.profile_arr[sprite_sys.profile_len] = (sprite_profile){ tex, x, y, w, h };

    log_debug("Made SpriteProfile [%u]: Texture = [%u]; x = %.0f; y = %.0f; w = %.0f; h = %.0f", sprite_sys.profile_len, tex, x, y, w, h);

    return sprite_sys.profile_len++;
}

sprite_profile *sprite_profile_get(usize idx) {
    if (idx == 0 || idx >= sprite_sys.profile_len) {
        log_err("sprite_profile_get(): SpriteProfile [%u] is invalid => stub", idx);
        assert(false);
        return &sprite_sys.profile_arr[0];
    }
    return &sprite_sys.profile_arr[idx];
}

// =============================================================================
u32 sprite_create(u32 profile_idx, sprite **r_sprite) {
    if (sprite_sys.len >= sprite_sys.cap) {
        log_err("sprite_create(): too much sprites (%zu) => stub", sprite_sys.len);
        assert(false);
        return 0;
    }
    if (profile_idx == 0 || profile_idx >= sprite_sys.profile_len) {
        log_err("sprite_create(): SpriteProfile [%u] is invalid => stub", profile_idx);
        assert(false);
        profile_idx = 0;
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

    // setup sprite
    memset(spr, 0, sizeof(sprite));
    spr->pool_flag = ALIVE_POOL_FLAG;
    spr->idx = idx;

    sprite_profile spr_prf = sprite_sys.profile_arr[profile_idx];
    spr->tex = spr_prf.tex;
    spr->sx = spr_prf.x;
    spr->sy = spr_prf.y;
    spr->sw = spr_prf.w;
    spr->sh = spr_prf.h;

    log_debug("Created Sprite [%u]: SpriteProfile = [%u]", idx, profile_idx);

    if (r_sprite != nullptr) *r_sprite = spr;
    return idx;
}

void sprite_destroy(u32 idx) {
    // only remove from pool
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

sprite *sprite_get(u32 idx) {
    if (idx == 0 || idx >= sprite_sys.max_idx) {
        log_err("sprite_get(): sprite invalid => stub");
        assert(false);
        return &sprite_sys.sprite_pool[0];
    }
    return &sprite_sys.sprite_pool[idx];
}

// =============================================================================
void sprite_sys_draw() {
    for (usize i = 1; i < sprite_sys.max_idx; ++i) {
        sprite spr = sprite_sys.sprite_pool[i];
        if (spr.pool_flag != ALIVE_POOL_FLAG) continue;

        // setup drawer
        drawer *drr = draw_make();
        drr->tex = spr.tex;
        memcpy(&drr->sx, &spr.sx, 4 * sizeof(f32));
        drr->dx = spr.x;
        drr->dy = spr.y;
        drr->dw = spr.sw;
        drr->dh = spr.sh;
        draw_meta_set_y(&drr->meta, drr->dy);
        draw_meta_set_z(&drr->meta, spr.z);
    }
}
