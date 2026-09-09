#include "draw.h"

#include "context.h"
#include "log.h"
#include <raylib.h>

struct draw_sys draw_sys = {0};

// =============================================================================
void draw_sys_init(usize texture_cap, usize drawer_cap) {
    // textures
    draw_sys.tex_arr = arena_alloc(&omni_arena, texture_cap * sizeof(Texture2D));
    draw_sys.tex_cap = texture_cap;
    draw_sys.tex_len = 0;

    if (drawer_cap > DRAW_MAX_CAP) {
        log_warn("draw_init(): drawer_cap too big => drawer_cap = DRAW_MAX_CAP");
        drawer_cap = DRAW_MAX_CAP;
    }

    // drawers
    draw_sys.drawer_buffer[0] = arena_alloc(&omni_arena, drawer_cap * sizeof(drawer));
    draw_sys.drawer_buffer[1] = arena_alloc(&omni_arena, drawer_cap * sizeof(drawer));
    draw_sys.drawer_cap = drawer_cap;
    draw_sys.drawer_len = 0;

    // stub
    draw_sys.tex_len = 1;

    // make error texture`
    Image stub_img = GenImageChecked(16, 16, 8, 8, MAGENTA, BLACK);
    Texture stub_tex = LoadTextureFromImage(stub_img);
    SetTextureFilter(stub_tex, TEXTURE_FILTER_POINT);
    UnloadImage(stub_img);

    ((Texture2D *)draw_sys.tex_arr)[0] = stub_tex;
}

// =============================================================================
u32 texture_load(const char *path) {
    char abs_path[256];
    strcpy(abs_path, ASSET_DIR);
    strcat(abs_path, "/");
    strcat(abs_path, path);

    Texture2D tex = LoadTexture(abs_path);
    if (!IsTextureValid(tex)) {
        log_err("Failed to load Texture \"%s\" => stub", abs_path);
        return 0;
    }

    ((Texture2D *)draw_sys.tex_arr)[draw_sys.tex_len] = tex;

    log_debug("Loaded Texture [%u] from \"%s\"", draw_sys.tex_len, abs_path);

    return draw_sys.tex_len++;
}

// =============================================================================
drawer *draw_make() {
    if (draw_sys.drawer_len >= draw_sys.drawer_cap) {
        log_err("draw_make(): too much draw call => null");
        return nullptr;
    }

    // always use buffer[0] first
    drawer *drr = &draw_sys.drawer_buffer[0][draw_sys.drawer_len];
    memset(drr, 0, sizeof(drawer));

    ++draw_sys.drawer_len;

    return drr;
}

void draw_meta_set_z(u64 *meta, i8 z) {
    // i8 to u8
    u8 uz = (u8)(z ^ 0x80);

    *meta &= ~(0xFFull << (64 - 8));
    *meta |= (u64)uz << (64 - 8);
}
void draw_meta_set_y(u64 *meta, f32 y) {
    // f32 to u32
    u32 uy; memcpy(&uy, &y, sizeof(f32));
    uy ^= (-(i32)(uy >> 31) | 0x80000000u);

    *meta &= ~(0xFFFFFFFFull << (64 - 8 - 32));
    *meta |= (u64)uy << (64 - 8 - 32);
}

#define RADIX_BITS 8
void draw_present() {
    usize cur_buffer = 0;

    // radix sort (on meta: z -> y -> ...)
    for (usize shift = 0; shift < 64; shift += RADIX_BITS) {
        u32 cnt[1 << RADIX_BITS] = {0};
        u32 offs[1 << RADIX_BITS];

        for (usize i = 0; i < draw_sys.drawer_len; ++i) {
            drawer *drr = &draw_sys.drawer_buffer[cur_buffer][i];
            usize idx = (drr->meta >> shift) & ((1 << RADIX_BITS) - 1);
            ++cnt[idx];
        }

        if (cnt[0] == draw_sys.drawer_len) {
            continue;
        }

        offs[0] = 0;
        for (usize i = 1; i < (1 << RADIX_BITS); ++i) {
            offs[i] = offs[i - 1] + cnt[i - 1];
        }

        drawer *dest_buffer = draw_sys.drawer_buffer[1 - cur_buffer];
        for (usize i = 0; i < draw_sys.drawer_len; ++i) {
            drawer *drr = &draw_sys.drawer_buffer[cur_buffer][i];
            usize idx = (drr->meta >> shift) & ((1 << RADIX_BITS) - 1);

            dest_buffer[offs[idx]++] = *drr;
        }

        cur_buffer = 1 - cur_buffer;
    }

    // actual draw
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
