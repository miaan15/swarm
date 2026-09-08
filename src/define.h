#pragma once

#include <stdint.h>
#include <stddef.h>

        //                  //
typedef int8_t              i8;
typedef int16_t             i16;
typedef int32_t             i32;
typedef int64_t             i64;

typedef uint8_t             u8;
typedef uint16_t            u16;
typedef uint32_t            u32;
typedef uint64_t            u64;

typedef float               f32;
typedef double              f64;
typedef long double         f128;

typedef size_t              usize;
typedef ptrdiff_t           isize;
typedef uintptr_t           uptr;
typedef intptr_t            iptr;

//
static constexpr const char PROJECT_DIR[] = _PROJECT_DIR;
static constexpr const char SRC_DIR[] = _PROJECT_DIR "/src";
static constexpr const char ASSET_DIR[] = _PROJECT_DIR "/asset";

//
#define ALIVE_POOL_FLAG ((u32)-1)

//
[[nodiscard]] static inline size_t align_up(size_t base, size_t align) {
    return (base + align - 1) & ~(align - 1);
}
