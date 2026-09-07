#include "context.h"

#include "entity.h"
#include "draw.h"
#include "sprite.h"

arena omni_arena = {0};

void engine_init() {
    arena_init(&omni_arena, 100 << 10 << 10); // 100MB

    draw_init(1024, 8192);
    sprite_sys_init(256, 1024);
    entity_sys_init(2048);
}

void engine_update() {

}

void engine_input() {

}

void engine_draw() {

}
