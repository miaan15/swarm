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

    chunk_add_portrait(idx);

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

    chunk_remv_portrait(idx);

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
void portrait_sys_update() {
    for (usize i = 1; i < portrait_sys.portrait_max_idx; ++i) {
        portrait *potr = &portrait_sys.portrait_pool[i];
        if (potr->pool_flag != ALIVE_POOL_FLAG) continue;

        chunk_update_portrait(i);
    }

    f32 cam_x = camera_x - screen_width  / camera_zoom / 2;
    f32 cam_y = camera_y - screen_height / camera_zoom / 2;
    f32 cam_w = screen_width  / camera_zoom;
    f32 cam_h = screen_height / camera_zoom;
    u32 *potr_list; usize potr_list_len;
    chunk_query_portrait(cam_x, cam_y, cam_w, cam_h, &potr_list, &potr_list_len);

    for (usize i = 0; i < potr_list_len; ++i) {
        portrait *potr = &portrait_sys.portrait_pool[potr_list[i]];

        // setup drawer
        drawer *drr = draw_make();

        drr->tex = potr->tex;
        drr->sx = potr->sx;
        drr->sy = potr->sy;
        drr->sw = potr->sw;
        drr->sh = potr->sh;

        drr->dx = potr->x;
        drr->dy = potr->y;
        drr->dw = potr->w;
        drr->dh = potr->h;

        if (potr->entity_idx != 0) {
            // entity z
            entity *ett = entity_get(potr->entity_idx);
            u8 ett_uz = (u8)ett->z ^ 0x80;
            drr->meta |= (u64)ett_uz << 56;

            // entity y
            // f32 to u32
            f32 ett_y; entity_pos_get(potr->entity_idx, nullptr, &ett_y, nullptr, nullptr);
            u32 ett_uy; memcpy(&ett_uy, &ett_y, sizeof(f32));
            ett_uy ^= (-(i32)(ett_uy >> 31) | 0x80000000u);
            drr->meta |= (u64)ett_uy << 24;

            // potrait z in entity
            u8 potr_uz = (u8)potr->z_in_entity ^ 0x80;
            drr->meta |= (u64)potr_uz << 16;

            // 16 first bit of tex
            drr->meta |= (u64)(u16)potr->tex << 0;
        }
    }
}
