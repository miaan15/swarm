// memory stuff: allocator, alignment,...
// - mostly just arena allocator here

module;

#include <cstddef>
#include <cstdlib>
#include <cstring>

export module mem;

import def;

export namespace sw {

constexpr usize DEFAULT_ALIGNMENT = alignof(max_align_t);

struct arena {
    void *buffer_ptr;
    usize cap;
    usize buffer_head_offs;
};

usize align_forward(usize ptr, usize align) {
    usize mod = ptr & (align - 1);
    if (mod != 0) {
        ptr += (align - mod);
    }
    return ptr;
}

void arena_init(arena *a, usize cap) {
    a->buffer_ptr = malloc(cap);
    a->cap = cap;
    a->buffer_head_offs = 0;
}

void arena_destroy(arena *a) {
    if (a->buffer_ptr != nullptr) {
        free(a->buffer_ptr);
    }
    *a = {};
}

void *arena_alloc_raw_aligned(arena *a, usize size, usize align) {
    usize offs = static_cast<usize>(align_forward(a->buffer_head_offs, align));
    if (offs + size > a->cap) {
        return nullptr;
    }
    a->buffer_head_offs = offs + size;
    return static_cast<char*>(a->buffer_ptr) + offs;
}

void *arena_alloc_raw(arena *a, usize size) {
    return arena_alloc_raw_aligned(a, size, DEFAULT_ALIGNMENT);
}

void *arena_alloc_aligned(arena *a, usize size, usize align) {
    void *ptr = arena_alloc_raw_aligned(a, size, align);
    if (ptr != nullptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void *arena_alloc(arena *a, usize size) {
    void *ptr = arena_alloc_raw(a, size);
    if (ptr != nullptr) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void arena_reset(arena *a) {
    a->buffer_head_offs = 0;
}

void arena_init_over(arena *a, void *base, usize cap) {
    a->buffer_ptr = base;
    a->cap = cap;
    a->buffer_head_offs = 0;
}

void arena_init_in_arena(arena *a, arena *base_ar, usize cap) {
    arena_init_over(a, arena_alloc_raw(base_ar, cap), cap);
}

}
