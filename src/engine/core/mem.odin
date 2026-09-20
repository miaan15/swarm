package engine_core

import "core:mem"

arena :: struct {
    raw: rawptr,
    cap: u32,
    offs: u32,
}

arena_init :: proc(arn: ^arena, cap: u32) {
    buffer, err := mem.alloc(int(cap))
    arn.raw = buffer
    arn.cap = cap
    arn.offs = 0
}

arena_destroy :: proc(arn: ^arena) {
    if arn.raw != nil { mem.free(arn.raw) }
    arn^ = {}
}

arena_alloc_raw_aligned :: proc(arn: ^arena, size: u32, align: u32) -> rawptr {
    offs := u32(mem.align_forward_uint(uint(arn.offs), uint(align)))
    if offs + size > arn.cap { return nil }
    arn.offs = offs + size
    return rawptr(uintptr(arn.raw) + uintptr(offs))
}

arena_alloc_raw :: proc(arn: ^arena, size: u32) -> rawptr {
    return arena_alloc_raw_aligned(arn, size, u32(mem.DEFAULT_ALIGNMENT))
}

arena_alloc_aligned :: proc(arn: ^arena, size: u32, align: u32) -> rawptr {
    ptr := arena_alloc_raw_aligned(arn, size, align)
    mem.zero(ptr, int(size))
    return ptr
}

arena_alloc :: proc(arn: ^arena, size: u32) -> rawptr {
    ptr := arena_alloc_raw(arn, size)
    mem.zero(ptr, int(size))
    return ptr
}

arena_reset :: proc(arn: ^arena) {
    arn.offs = 0
}

arena_init_over :: proc(arn: ^arena, base: rawptr, cap: u32) {
    arn.raw = base
    arn.cap = cap
    arn.offs = 0
}

arena_init_in_arena :: proc(arn: ^arena, base_ar: ^arena, cap: u32) {
    arena_init_over(arn, arena_alloc_raw(base_ar, cap), cap)
}
