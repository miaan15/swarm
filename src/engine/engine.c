#include "context.h"

arena omni_arena = {0};

void engine_init() {
    arena_init(&omni_arena, 100 << 10 << 10); // 100MB
}
