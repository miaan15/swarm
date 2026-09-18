package engine

import "core:mem"
import "../global"
import "../engine/core"
import "core:strings"
import "core:path/filepath"
import sdl "vendor:sdl3"
import img "vendor:sdl3/image"

// TEXTURE
// ================================================================================================
texture_sys : struct {
    list: [^]^sdl.Texture,
    cap, len: u32,
} = {}

texture_sys_init :: proc(cap: u32) {
    texture_sys.list = transmute([^]^sdl.Texture)core.arena_alloc(&global.omni_arena, cap * size_of(^sdl.Texture))
    texture_sys.cap = cap

    // stub
    WIDTH  :: 32
    HEIGHT :: 32
    pixels: [WIDTH * HEIGHT]u32

    for y in 0..<HEIGHT {
        for x in 0..<WIDTH {
            pixels[y * WIDTH + x] = ((x / 16) + (y / 16)) % 2 == 0 ? 0xFFFF00FF : 0xFF000000
        }
    }

    stub_tex := sdl.CreateTexture(
        global.renderer,
        sdl.PixelFormat.RGBA8888,
        sdl.TextureAccess.STATIC,
        WIDTH, HEIGHT,
    )

    sdl.UpdateTexture(stub_tex, nil, raw_data(pixels[:]), WIDTH * size_of(u32))
    sdl.SetTextureScaleMode(stub_tex, .NEAREST)

    texture_sys.list[0] = stub_tex
    texture_sys.len = 1
}

texture_sys_destroy :: proc() {
    for i in 0..<texture_sys.len {
        sdl.DestroyTexture(texture_sys.list[i])
    }
}

texture_load :: proc(path: string, scale_mode: sdl.ScaleMode = sdl.ScaleMode.NEAREST) -> u32 {
    if (texture_sys.len >= texture_sys.cap) {
        core.log_error("texture_load: too much textures (%d) => stub", texture_sys.len)
        return 0
    }

    full_path, _ := filepath.join({global.asset_dir, path}, context.temp_allocator)
    full_path_cstr, _ := strings.clone_to_cstring(path, context.temp_allocator)

    tex := img.LoadTexture(global.renderer, full_path_cstr)
    if tex == nil {
        core.log_error("texture_load: failed to load texture: %s => stub", sdl.GetError())
        return 0
    }
    sdl.SetTextureScaleMode(tex, scale_mode)

    idx := texture_sys.len
    texture_sys.len += 1

    texture_sys.list[idx] = tex

    return idx
}

texture_get :: proc(idx: u32) -> ^sdl.Texture {
    if (idx == 0 || idx >= texture_sys.len) {
        core.log_warn("texture_get: texture [%d] invalid => stub", idx)
        return texture_sys.list[0]
    }
    return texture_sys.list[idx]
}

// DRAW
// ================================================================================================
draw :: struct {
    meta: u64,

    type: enum { TEXTURE, BOX, RECTANGLE, },
    using _: struct #raw_union {
        texture : struct {
            idx: u32,
            src, dest: [4]f32,
        },
        box: struct {
            rect: [4]f32,
            color: [4]u8,
        },
        rectangle : struct {
            rect: [4]f32,
            thickness: f32,
            color: [4]u8
        },
    }
}

draw_sys : struct {
    buffer: [2]([^]draw),
    cap, len: u32,
} = {}

draw_init :: proc(cap: u32) {
    draw_sys.buffer[0] = transmute([^]draw)core.arena_alloc(&global.omni_arena, cap * size_of(draw))
    draw_sys.buffer[1] = transmute([^]draw)core.arena_alloc(&global.omni_arena, cap * size_of(draw))
    draw_sys.cap = cap
    draw_sys.len = 0
}

draw_make :: proc() -> ^draw {
    if (draw_sys.len >= draw_sys.cap) {
        core.log_error("draw_make: too much draws (%d) => nil", draw_sys.len)
        return nil
    }

    idx := draw_sys.len
    draw_sys.len += 1

    drw := &draw_sys.buffer[0][idx]
    mem.zero(drw, size_of(draw))

    return drw
}

draw_present :: proc() {
    RADIX_SORT_BITS :: 8
    RADIX_SORT_COUNT :: 1 << RADIX_SORT_BITS

    buffer_idx := 0

    // radix sort by .meta -> .type
    { // by .type first
        counts: [1 << 8]u32
        offs: [1 << 8]u32

        for i in 0..<draw_sys.len {
            slot := u8(draw_sys.buffer[buffer_idx][i].type) & ((1 << 8) - 1)

            counts[slot] += 1
        }

        offs[0] = 0
        for i in 1..<(1 << 8) {
            offs[i] = offs[i - 1] + counts[i - 1]
        }

        for i in 0..<draw_sys.len {
            slot := u8(draw_sys.buffer[buffer_idx][i].type) & ((1 << 8) - 1)

            assert(offs[slot] < draw_sys.len)
            draw_sys.buffer[1 - buffer_idx][offs[slot]] = draw_sys.buffer[buffer_idx][i]

            offs[slot] += 1
        }

        buffer_idx = 1 - buffer_idx
    }
    for shift: u64 = 0; shift < 64; shift += RADIX_SORT_BITS {
        counts: [RADIX_SORT_COUNT]u32
        offs: [RADIX_SORT_COUNT]u32

        for i in 0..<draw_sys.len {
            slot := (draw_sys.buffer[buffer_idx][i].meta >> shift) & ((1 << RADIX_SORT_BITS) - 1)
            assert(slot < RADIX_SORT_COUNT)

            counts[slot] += 1
        }

        offs[0] = 0
        for i in 1..<RADIX_SORT_COUNT {
            offs[i] = offs[i - 1] + counts[i - 1]
        }

        for i in 0..<draw_sys.len {
            slot := (draw_sys.buffer[buffer_idx][i].meta >> shift) & ((1 << RADIX_SORT_BITS) - 1)
            assert(slot < RADIX_SORT_COUNT)

            assert(offs[slot] < draw_sys.len)
            draw_sys.buffer[1 - buffer_idx][offs[slot]] = draw_sys.buffer[buffer_idx][i]

            offs[slot] += 1
        }

        buffer_idx = 1 - buffer_idx
    }

    // draw
    draw_buffer := draw_sys.buffer[buffer_idx]
    for i in 0..<draw_sys.len {
        drw := draw_buffer[i]

        switch drw.type {
        case .TEXTURE:
            tex := texture_get(drw.texture.idx)

            src_rect := sdl.FRect{drw.texture.src.x, drw.texture.src.y, drw.texture.src.z, drw.texture.src.w}
            dst_rect := sdl.FRect{drw.texture.dest.x, drw.texture.dest.y, drw.texture.dest.z, drw.texture.dest.w}

            src_rect_ptr := &src_rect if (src_rect.w > 0 && src_rect.h > 0) else nil

            sdl.RenderTexture(global.renderer, tex, src_rect_ptr, &dst_rect)

        case .BOX:
            sdl.SetRenderDrawColor(
                global.renderer,
                drw.box.color.r,
                drw.box.color.g,
                drw.box.color.b,
                drw.box.color.a,
            )

            rect := sdl.FRect{drw.box.rect.x, drw.box.rect.y, drw.box.rect.z, drw.box.rect.w}

            sdl.RenderFillRect(global.renderer, &rect)

        case .RECTANGLE:
            sdl.SetRenderDrawColor(
                global.renderer,
                drw.rectangle.color.r,
                drw.rectangle.color.g,
                drw.rectangle.color.b,
                drw.rectangle.color.a,
            )

            rect := sdl.FRect{drw.rectangle.rect.x, drw.rectangle.rect.y, drw.rectangle.rect.z, drw.rectangle.rect.w}

            if drw.rectangle.thickness <= 1 {
                sdl.RenderRect(global.renderer, &rect)
            } else {
                t := drw.rectangle.thickness
                borders := [4]sdl.FRect{
                    {rect.x, rect.y, rect.w, t},
                    {rect.x, rect.y + rect.h - t, rect.w, t},
                    {rect.x, rect.y + t, t, rect.h - (t * 2)},
                    {rect.x + rect.w - t, rect.y + t, t, rect.h - (t * 2)},
                }
                sdl.RenderFillRects(global.renderer, raw_data(borders[:]), len(borders))
            }
        }
    }

    // reset
    draw_sys.len = 0
}
