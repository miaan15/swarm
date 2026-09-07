#pragma once

#include "define.h"

#define DRAW_MAX_CAP (((usize)1 << 32) - 1)

typedef struct {
    u64 meta;

    f32 sx, sy, sw, sh;
    f32 dx, dy, dw, dh;
    u32 tex;
} drawer;

struct draw_sys {
    void *tex_arr;
    usize tex_cap;
    usize tex_len;

    drawer *drawer_buffer[2];
    usize drawer_cap;
    usize drawer_len;
};

extern struct draw_sys draw_sys;

void draw_init(usize texture_cap, usize drawer_cap);

u32 texture_load(const char *dir);

drawer *drawer_make();

void drawer_meta_set_z(u64 *meta, i8 z);
void drawer_meta_set_y(u64 *meta, f32 y);

void draw_present();
