#include "portrait.h"

#include "context.h"
#include "draw.h"
#include "log.h"
#include <assert.h>

struct portrait_sys portrait_sys = {0};

// =============================================================================
void portrait_sys_init(usize profile_cap, usize portrait_cap) {
    // profiles
    portrait_sys.profile_arr = arena_alloc(&omni_arena, profile_cap * sizeof(portrait_profile));
    portrait_sys.profile_cap = profile_cap;

    // portraits
    portrait_sys.portrait_pool = arena_alloc(&omni_arena, portrait_cap * sizeof(portrait));
    portrait_sys.portrait_cap = portrait_cap;

    // stub
    portrait_sys.profile_len = 1;
    portrait_sys.profile_arr[0] = (portrait_profile){0};
    portrait_sys.profile_arr[0].w = 16;
    portrait_sys.profile_arr[0].h = 16;

    portrait_sys.portrait_head = portrait_sys.portrait_max_idx = portrait_sys.portrait_len = 1;
    portrait_sys.portrait_pool[0] = (portrait){0};
    portrait_sys.portrait_pool[0].w = portrait_sys.portrait_pool[0].sw = 16;
    portrait_sys.portrait_pool[0].h = portrait_sys.portrait_pool[0].sh = 16;
}

// =============================================================================
u32 portrait_profile_create(u32 tex, f32 x, f32 y, f32 w, f32 h) {
    if (tex == 0 || tex >= draw_sys.tex_len) {
        log_err("portrait_profile_create(): Texture [%u] is invalid => tex = stub", tex);
        assert(false);
        tex = 0;
    }

    portrait_sys.profile_arr[portrait_sys.profile_len] = (portrait_profile){ tex, x, y, w, h };

    log_debug("Made PortraitProfile [%u]: Texture = [%u]; x = %.0f; y = %.0f; w = %.0f; h = %.0f", portrait_sys.profile_len, tex, x, y, w, h);

    return portrait_sys.profile_len++;
}

portrait_profile *portrait_profile_get(usize idx) {
    if (idx == 0 || idx >= portrait_sys.profile_len) {
        log_err("portrait_profile_get(): PortraitProfile [%u] is invalid => stub", idx);
        assert(false);
        return &portrait_sys.profile_arr[0];
    }
    return &portrait_sys.profile_arr[idx];
}

// =============================================================================
u32 portrait_create(u32 profile_idx, portrait **r_portrait) {
    if (portrait_sys.portrait_len >= portrait_sys.portrait_cap) {
        log_err("portrait_create(): too much portraits (%zu) => stub", portrait_sys.portrait_len);
        assert(false);
        return 0;
    }
    if (profile_idx == 0 || profile_idx >= portrait_sys.profile_len) {
        log_err("portrait_create(): PortraitProfile [%u] is invalid => stub", profile_idx);
        assert(false);
        profile_idx = 0;
    }

    usize idx = portrait_sys.portrait_head;
    portrait *potr = &portrait_sys.portrait_pool[idx];

    if (idx == portrait_sys.portrait_max_idx) {
        ++portrait_sys.portrait_max_idx;
        ++portrait_sys.portrait_head;
    } else {
        portrait_sys.portrait_head = potr->pool_flag;
    }

    ++portrait_sys.portrait_len;

    // setup portrait
    memset(potr, 0, sizeof(portrait));
    potr->pool_flag = ALIVE_POOL_FLAG;
    potr->idx = idx;

    portrait_profile potr_prf = portrait_sys.profile_arr[profile_idx];
    potr->tex = potr_prf.tex;
    potr->sx = potr_prf.x;
    potr->sy = potr_prf.y;
    potr->sw = potr_prf.w;
    potr->sh = potr_prf.h;

    potr->w = potr->sw;
    potr->h = potr->sh;

    log_debug("Created Portrait [%u]: PortraitProfile = [%u]", idx, profile_idx);

    if (r_portrait != nullptr) *r_portrait = potr;
    return idx;
}

void portrait_destroy(u32 idx) {
    // only remove from pool
    portrait *potr = &portrait_sys.portrait_pool[idx];

    if (potr->pool_flag != ALIVE_POOL_FLAG) {
        log_warn("portrait_destroy(): portrait already dead");
        return;
    }

    potr->pool_flag = portrait_sys.portrait_head;
    portrait_sys.portrait_head = idx;

    --portrait_sys.portrait_len;

    log_debug("Destroyed Portrait [%u]", idx);
}

portrait *portrait_get(u32 idx) {
    if (idx == 0 || idx >= portrait_sys.portrait_max_idx) {
        log_err("portrait_get(): portrait invalid => stub");
        assert(false);
        return &portrait_sys.portrait_pool[0];
    }
    return &portrait_sys.portrait_pool[idx];
}

// =============================================================================
void portrait_sys_draw() {
    for (usize i = 1; i < portrait_sys.portrait_max_idx; ++i) {
        portrait *potr = &portrait_sys.portrait_pool[i];
        if (potr->pool_flag != ALIVE_POOL_FLAG) continue;

        if (potr->show || true) { // FIXME
            // setup drawer
            drawer *drr = draw_make();
            drr->tex = potr->tex;
            memcpy(&drr->sx, &potr->sx, 4 * sizeof(f32));
            drr->dx = potr->x;
            drr->dy = potr->y;
            drr->dw = potr->w;
            drr->dh = potr->h;
            draw_meta_set_y(&drr->meta, drr->dy);
            draw_meta_set_z(&drr->meta, potr->z);

            // hid again
            potr->show = false;
        }
    }
}
