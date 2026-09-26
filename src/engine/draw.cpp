// engine main way of drawing to screen, also include texture and stuff
// - texture: load from whatever SDL_image support, in assets/ dir
// - draw is immediate: draw need to call every frame
// - supposed to manually edit draw_call pointer after making it
// - draw_call.sorting (u64) used to sort out for the final render

module;

#include <cassert>
#include <cstdio>
#include <cstring>
#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>

export module draw;

import def;
import mem;
import context;
import log;

export namespace sw {

// ================================================================================================
// TEXTURE
// ================================================================================================

struct {
    SDL_Texture **texture_list_ptr;
    u32 texture_cap;
    u32 texture_list_len;
} texture_sys = {};

void texture_sys_init(u32 texture_cap) {
    texture_sys.texture_list_ptr = (SDL_Texture**)arena_alloc(&omni_arena, texture_cap * sizeof(SDL_Texture*));
    texture_sys.texture_cap = texture_cap;

    // stub - pink-black checkerboard texture
    constexpr u32 WIDTH  = 32;
    constexpr u32 HEIGHT = 32;
    u32 pixels[WIDTH * HEIGHT];

    for (u32 y = 0; y < HEIGHT; ++y) {
        for (u32 x = 0; x < WIDTH; ++x) {
            pixels[y * WIDTH + x] = (((x / 16) + (y / 16)) % 2 == 0) ? 0xFFFF00FF : 0xFF000000;
        }
    }

    SDL_Texture *stub_tex = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STATIC,
        WIDTH,
        HEIGHT
    );

    SDL_UpdateTexture(stub_tex, nullptr, pixels, WIDTH * sizeof(u32));
    SDL_SetTextureScaleMode(stub_tex, SDL_SCALEMODE_NEAREST);

    texture_sys.texture_list_ptr[0] = stub_tex;
    texture_sys.texture_list_len = 1;
}

/**
 * @param path texture relative path to assets_dir
 * @param scale_mode SDL texture filtering mode (default nearest)
 * @return texture handle index (0 on failure / stub)
 */
u32 texture_load(const char *path, SDL_ScaleMode scale_mode = SDL_SCALEMODE_NEAREST) {
    // load texture using SDL, path is relative to assets_dir

    if (texture_sys.texture_list_len >= texture_sys.texture_cap) {
        log_err("texture_load: too many textures (%u) => stub", texture_sys.texture_list_len);
        return 0;
    }

    // get full path to asset
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s%s%s", assets_dir, PATH_SEPARATOR, path);

    // load use SDL
    SDL_Texture *tex = IMG_LoadTexture(renderer, full_path);
    if (!tex) {
        log_err("texture_load: failed to load texture: %s => stub", SDL_GetError());
        return 0;
    }
    SDL_SetTextureScaleMode(tex, scale_mode);

    // append to texture list
    u32 idx = texture_sys.texture_list_len;
    texture_sys.texture_list_len++;

    texture_sys.texture_list_ptr[idx] = tex;

    log_debug("loaded texture [%u]: path = %s", idx, full_path);

    return idx;
}

/**
 * @param idx texture handle index
 * @return SDL_Texture pointer, or slot 0 stub texture if invalid
 */
SDL_Texture *texture_get(u32 idx) {
    if (idx == 0 || idx >= texture_sys.texture_list_len) {
        log_trace("texture_get: texture [%u] invalid => stub", idx);
        return texture_sys.texture_list_ptr[0];
    }
    return texture_sys.texture_list_ptr[idx];
}

// ================================================================================================
// DRAW
// ================================================================================================

enum struct draw_type : u8 {
    NIL = 0,
    TEXTURE, BOX, RECTANGLE,
};

struct draw_call {
    u64 sorting;
    draw_type type;

    // union data depend on .type
    union {
        struct {
            u32 texture_idx;
            f32 src_rect[4];
            f32 dest_rect[4];
        } texture;

        struct {
            f32 rect[4];
            u8 color[4];
        } box;

        struct {
            f32 rect[4];
            f32 thickness;
            u8 color[4];
        } rectangle;
    };
};

struct {
    draw_call *draw_buffers[2];
    // 2 draw buffer for radix sort algorithm, but default just use the index-0 buffer
    u32 draw_cap;
    u32 draw_len;

    draw_call stub_draw_call;
} draw_sys = {};

void draw_sys_init(u32 cap) {
    draw_sys.draw_buffers[0] = (draw_call*)arena_alloc(&omni_arena, cap * sizeof(draw_call));
    draw_sys.draw_buffers[1] = (draw_call*)arena_alloc(&omni_arena, cap * sizeof(draw_call));
    draw_sys.draw_cap = cap;
    draw_sys.draw_len = 0;
}

/**
 * @brief allocate a new draw command in the current frame buffer
 * @return pointer to zeroed struct to fill in, or stub on overflow
 */
draw_call *draw_call_make() {
    // create a draw call, the returned pointer is supposed to be modified make actual draw

    if (draw_sys.draw_len >= draw_sys.draw_cap) {
        log_err("draw_call_make: too many draws (%u) => nil", draw_sys.draw_len);
        return &draw_sys.stub_draw_call;
    }

    u32 idx = draw_sys.draw_len;
    draw_sys.draw_len++;

    // append new draw command
    draw_call *drw = &draw_sys.draw_buffers[0][idx];
    memset(drw, 0, sizeof(draw_call));

    return drw;
}

void draw_sys_present() {
    constexpr u32 RADIX_SORT_BITS  = 8;
    constexpr u32 RADIX_SORT_COUNT = 1 << RADIX_SORT_BITS;

    u32 buffer_idx = 0;

    // radix sort pass on .type to batch pipeline state
    {
        u32 counts[RADIX_SORT_COUNT] = {};
        u32 offs[RADIX_SORT_COUNT]   = {};

        // count histogram
        for (u32 i = 0; i < draw_sys.draw_len; ++i) {
            u8 slot = (u8)draw_sys.draw_buffers[buffer_idx][i].type;
            counts[slot]++;
        }

        // prefix sum to get bucket write offsets
        offs[0] = 0;
        for (u32 i = 1; i < RADIX_SORT_COUNT; ++i) {
            offs[i] = offs[i - 1] + counts[i - 1];
        }

        // scatter to other buffer
        for (u32 i = 0; i < draw_sys.draw_len; ++i) {
            u8 slot = (u8)draw_sys.draw_buffers[buffer_idx][i].type;
            assert(offs[slot] < draw_sys.draw_len);
            draw_sys.draw_buffers[1 - buffer_idx][offs[slot]] = draw_sys.draw_buffers[buffer_idx][i];
            offs[slot]++;
        }

        // swap buffer
        buffer_idx = 1 - buffer_idx;
    }

    // radix sort on .sorting
    for (u32 shift = 0; shift < 64; shift += RADIX_SORT_BITS) {
        u32 counts[RADIX_SORT_COUNT] = {};
        u32 offs[RADIX_SORT_COUNT]   = {};

        // count histogram
        for (u32 i = 0; i < draw_sys.draw_len; ++i) {
            u32 slot = (u32)((draw_sys.draw_buffers[buffer_idx][i].sorting >> shift) & (RADIX_SORT_COUNT - 1));
            assert(slot < RADIX_SORT_COUNT);
            counts[slot]++;
        }

        // prefix sum to get bucket write offsets
        offs[0] = 0;
        for (u32 i = 1; i < RADIX_SORT_COUNT; ++i) {
            offs[i] = offs[i - 1] + counts[i - 1];
        }

        // scatter to other buffer
        for (u32 i = 0; i < draw_sys.draw_len; ++i) {
            u32 slot = (u32)((draw_sys.draw_buffers[buffer_idx][i].sorting >> shift) & (RADIX_SORT_COUNT - 1));
            assert(slot < RADIX_SORT_COUNT);
            assert(offs[slot] < draw_sys.draw_len);
            draw_sys.draw_buffers[1 - buffer_idx][offs[slot]] = draw_sys.draw_buffers[buffer_idx][i];
            offs[slot]++;
        }

        // swap buffer
        buffer_idx = 1 - buffer_idx;
    }

    // actual draw
    draw_call *draw_buffer = draw_sys.draw_buffers[buffer_idx];
    for (u32 i = 0; i < draw_sys.draw_len; ++i) {
        draw_call *drw = &draw_buffer[i];

        switch (drw->type) {
            case draw_type::TEXTURE: {
                SDL_Texture *tex = texture_get(drw->texture.texture_idx);

                SDL_FRect src_rect  = { drw->texture.src_rect[0],  drw->texture.src_rect[1],  drw->texture.src_rect[2],  drw->texture.src_rect[3] };
                SDL_FRect dest_rect = { drw->texture.dest_rect[0], drw->texture.dest_rect[1], drw->texture.dest_rect[2], drw->texture.dest_rect[3] };

                const SDL_FRect *src_rect_ptr = (src_rect.w > 0.0f && src_rect.h > 0.0f) ? &src_rect : nullptr;

                SDL_RenderTexture(renderer, tex, src_rect_ptr, &dest_rect);
                break;
            }

            case draw_type::BOX: {
                SDL_SetRenderDrawColor(
                    renderer,
                    drw->box.color[0],
                    drw->box.color[1],
                    drw->box.color[2],
                    drw->box.color[3]
                );

                SDL_FRect rect = { drw->box.rect[0], drw->box.rect[1], drw->box.rect[2], drw->box.rect[3] };
                SDL_RenderFillRect(renderer, &rect);
                break;
            }

            case draw_type::RECTANGLE: {
                SDL_SetRenderDrawColor(
                    renderer,
                    drw->rectangle.color[0],
                    drw->rectangle.color[1],
                    drw->rectangle.color[2],
                    drw->rectangle.color[3]
                );

                SDL_FRect rect = { drw->rectangle.rect[0], drw->rectangle.rect[1], drw->rectangle.rect[2], drw->rectangle.rect[3] };

                if (drw->rectangle.thickness <= 1.0f) {
                    SDL_RenderRect(renderer, &rect);
                } else {
                    f32 t = drw->rectangle.thickness;
                    SDL_FRect borders[4] = {
                        { rect.x, rect.y, rect.w, t },
                        { rect.x, rect.y + rect.h - t, rect.w, t },
                        { rect.x, rect.y + t, t, rect.h - (t * 2.0f) },
                        { rect.x + rect.w - t, rect.y + t, t, rect.h - (t * 2.0f) }
                    };
                    SDL_RenderFillRects(renderer, borders, 4);
                }
                break;
            }

            default: break;
        }
    }

    draw_sys.draw_len = 0;
    draw_sys.stub_draw_call = {};
}

}
