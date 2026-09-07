#pragma once

#include "define.h"
#include <stdalign.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    void *raw;
    usize cap;
    usize offs;
} arena;

static inline void arena_init(arena *ar, usize cap) {
    ar->raw = malloc(cap);
    ar->cap = cap;
    ar->offs = 0;
}

static inline void arena_destroy(arena *ar) {
    if (ar->raw) free(ar->raw);
    memset(ar, 0, sizeof(arena));
}

static inline void *arena_alloc_raw(arena *ar, usize size) {
    usize offs = align_up(ar->offs, alignof(max_align_t));
    if (offs + size > ar->cap) { return NULL; }
    ar->offs = offs + size;
    return (char *)ar->raw + offs;
}

static inline void *arena_alloc(arena *ar, usize size) {
    void *ptr = arena_alloc_raw(ar, size);
    memset(ptr, 0, size);
    return ptr;
}

static inline void arena_reset(arena *ar) {
    ar->offs = 0;
}

static inline void arena_init_over(arena *ar, void *base, usize cap) {
    ar->raw = base;
    ar->cap = cap;
    ar->offs = 0;
}

static inline void arena_init_in_arena(arena *ar, arena *base_ar, usize cap) {
    arena_init_over(ar, arena_alloc_raw(base_ar, cap), cap);
}
