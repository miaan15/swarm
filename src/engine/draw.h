#pragma once

#include "define.h"

#define DRAW_MAX_CAP (((usize)1 << 32) - 1)

typedef struct {
    u64 meta;

    u32 tex;
    f32 sx, sy, sw, sh;
    f32 dx, dy, dw, dh;
} drawer;

// draw_sys contains both texture (wrapper of raylib Texture2D) and drawer (draw call instance in a frame)
struct draw_sys {
    void *tex_arr;
    usize tex_cap;
    usize tex_len;

    drawer *drawer_buffer[2];
    usize drawer_cap;
    usize drawer_len;
};
extern struct draw_sys draw_sys;

// =============================================================================
void draw_sys_init(usize texture_cap, usize drawer_cap);
void draw_sys_destroy();

// =============================================================================
u32 texture_load(const char *path);

// =============================================================================
drawer *draw_make();

void draw_meta_set_z(u64 *meta, i8 z);
void draw_meta_set_y(u64 *meta, f32 y);

void draw_present();
