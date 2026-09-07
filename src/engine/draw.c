#include "draw.h"

#include "context.h"
#include "log.h"
#include <raylib.h>

struct draw_sys draw_sys = {0};

void draw_init(usize texture_cap, usize drawer_cap) {
    draw_sys.tex_arr = arena_alloc(&omni_arena, texture_cap * sizeof(Texture2D));
    draw_sys.tex_cap = texture_cap;
    draw_sys.tex_len = 0;

    if (drawer_cap > DRAW_MAX_CAP) {
        log_warn("draw_init(): drawer_cap too big => drawer_cap = DRAW_MAX_CAP");
        drawer_cap = DRAW_MAX_CAP;
    }

    draw_sys.drawer_buffer[0] = arena_alloc(&omni_arena, drawer_cap * sizeof(drawer));
    draw_sys.drawer_buffer[1] = arena_alloc(&omni_arena, drawer_cap * sizeof(drawer));
    draw_sys.drawer_cap = drawer_cap;
    draw_sys.drawer_len = 0;
}

u32 texture_load(const char *dir) {
    ((Texture2D *)draw_sys.tex_arr)[draw_sys.tex_len] = LoadTexture(dir);
    return draw_sys.tex_len++;
}

drawer *drawer_make() {
    if (draw_sys.drawer_len >= draw_sys.drawer_cap) {
        log_err("draw_make(): too much drawers => null");
        return NULL;
    }
    drawer *drr = &draw_sys.drawer_buffer[0][draw_sys.drawer_len];
    drr->meta = draw_sys.drawer_len;
    ++draw_sys.drawer_len;
    return drr;
}

void drawer_meta_set_z(u64 *meta, i8 z) {
    *meta |= (u64)z << (64 - 8);
}
void drawer_meta_set_y(u64 *meta, f32 y) {
    u32 _y; memcpy(&_y, &y, sizeof(u32));

    _y = _y ^ (-((i32)_y >> 31) | (1 << 31));
    _y = ~_y;

    *meta |= (u64)_y << (64 - 8 - 32);
}

#define RADIX_BITS 8
void draw_present() {
    usize cur_buffer = 0;

    for (usize shift = 0; shift < 64; shift += RADIX_BITS) {
        u32 cnt[1 << RADIX_BITS] = {0};
        u32 offs[1 << RADIX_BITS];

        for (usize i = 0; i < draw_sys.drawer_len; ++i) {
            drawer *drr = &draw_sys.drawer_buffer[cur_buffer][i];
            usize idx = (drr->meta >> shift) & ((1 << RADIX_BITS) - 1);
            ++cnt[idx];
        }

        offs[0] = 0;
        for (usize i = 1; i < draw_sys.drawer_len; ++i) {
            offs[i] = offs[i - 1] + cnt[i - 1];
        }

        for (usize i = 0; i < draw_sys.drawer_len; ++i) {
            drawer *drr = &draw_sys.drawer_buffer[cur_buffer][i];
            usize idx = (drr->meta >> shift) & ((1 << RADIX_BITS) - 1);

            drawer *dest_buffer = draw_sys.drawer_buffer[1 - cur_buffer];
            dest_buffer[offs[idx]++] = *drr;
        }

        cur_buffer = 1 - cur_buffer;
    }

    drawer *draw_buffer = draw_sys.drawer_buffer[cur_buffer];
    for (usize i = 0; i < draw_sys.drawer_len; ++i) {
        drawer drawer = draw_buffer[i];

        Texture2D texture = ((Texture2D *)draw_sys.tex_arr)[drawer.tex];
        Rectangle src_rect = { drawer.sx, drawer.sy, drawer.sw, drawer.sh };
        Rectangle dest_rect = { drawer.dx, drawer.dy, drawer.dw, drawer.dh };

        DrawTexturePro(texture, src_rect, dest_rect, (Vector2){0}, 0.0f, WHITE);
    }

    draw_sys.drawer_len = 0;
}
