package game

import "core:math"
import "core:math/rand"
import "../engine/core"
import "../engine"
import "../entity"
import "../global"

NPC_COUNT :: 10000

NPC_MIN_X :: -9000
NPC_MAX_X ::  9000
NPC_MIN_Y :: -9000
NPC_MAX_Y ::  9000

Race :: enum u32 {
    Human,
    Elf,
    Dwarf,
}

HAIR_OFFSET_BY_RACES := [Race][2]f32{
    .Human = {0, 0},
    .Elf   = {0, 0},
    .Dwarf = {0, 3},
}

game_init :: proc() {
    engine.texture_sys_init(100)
    engine.draw_sys_init(1e5)

    entity.entity_sys_init(1e5)
    entity.sprite_sys_init(1e3, 1e5)
    entity.collider_sys_init(1e5, 16)

    engine.texture_load("img/char_base.png") // 1
    engine.texture_load("img/char_hair.png") // 2

    entity.sprite_profile_create(1, {   0, 0, 32, 32 }) // 1: human - male
    entity.sprite_profile_create(1, {  32, 0, 32, 32 }) // 2: human - female
    entity.sprite_profile_create(1, {  64, 0, 32, 32 }) // 3: elf - male
    entity.sprite_profile_create(1, {  96, 0, 32, 32 }) // 4: elf - female
    entity.sprite_profile_create(1, { 128, 0, 32, 32 }) // 5: dwarf - male
    entity.sprite_profile_create(1, { 160, 0, 32, 32 }) // 6: dwarf - female

    entity.sprite_profile_create(2, {   0, 0, 32, 32 }) // 7: male hair
    entity.sprite_profile_create(2, {  32, 0, 32, 32 }) // 8: male hair
    entity.sprite_profile_create(2, {  64, 0, 32, 32 }) // 9: female hair
    entity.sprite_profile_create(2, {  96, 0, 32, 32 }) // 10: female hair

    // scene
    for _ in 0..<NPC_COUNT {
        _, ett := entity.entity_create()

        ett.pos.x = rand.float32_range(NPC_MIN_X, NPC_MAX_X)
        ett.pos.y = rand.float32_range(NPC_MIN_Y, NPC_MAX_Y)

        race := rand.uint32() % 3
        gender := rand.uint32() % 2
        profile_idx := race * 2 + gender + 1

        entity.entity_new_sprite(ett.key, profile_idx, {-16, -16}, 0)

        entity.entity_new_collider(ett.key, {32, 32}, {-16, -16})

        speed := rand.float32_range(0, 1.0)
        angle := rand.float32_range(0, math.TAU)
        ett.vel.x = math.cos(angle) * speed
        ett.vel.y = math.sin(angle) * speed

        hair_profile_idx := gender * 2 + 1 + 6 + (rand.uint32() % 2)
        hair_offset := [2]f32{
            -16 + HAIR_OFFSET_BY_RACES[Race(race)].x,
            -16 + HAIR_OFFSET_BY_RACES[Race(race)].y,
        }
        entity.entity_new_sprite(ett.key, hair_profile_idx, hair_offset, 1)
    }

}

game_input :: proc() { }
game_update :: proc() {
}
game_update_late :: proc() { }
game_visual :: proc() {
    core.log_info("fps: %f", 1 / global.time_delta_s)
}
game_draw :: proc() {
    // entity._collider_debug_draw()
}
