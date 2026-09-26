module;

#include <cmath>
#include <random>

export module game;

import def;
import log;
import context;
import draw;
import entity;

export namespace sw {

constexpr u32 NPC_COUNT = 10'000;

constexpr f32 NPC_MIN_X = -10000.0f;
constexpr f32 NPC_MAX_X =  12800.0f;
constexpr f32 NPC_MIN_Y = -10000.0f;
constexpr f32 NPC_MAX_Y =   7200.0f;

enum struct Race : u32 {
    Human,
    Elf,
    Dwarf,
};

constexpr f32 HAIR_OFFSET_BY_RACES[3][2] = {
    {0.0f, 0.0f}, // Human
    {0.0f, 0.0f}, // Elf
    {0.0f, 3.0f}, // Dwarf
};

void game_init() {
    texture_sys_init(100);
    draw_sys_init(100'000);

    entity_sys_init(100'000);
    sprite_sys_init(1'000, 100'000);
    collider_sys_init(100'000, 16.0f);

    texture_load("img/char_base.png"); // 1
    texture_load("img/char_hair.png"); // 2

    u32 rect_base1[4] = {   0, 0, 32, 32 };
    u32 rect_base2[4] = {  32, 0, 32, 32 };
    u32 rect_base3[4] = {  64, 0, 32, 32 };
    u32 rect_base4[4] = {  96, 0, 32, 32 };
    u32 rect_base5[4] = { 128, 0, 32, 32 };
    u32 rect_base6[4] = { 160, 0, 32, 32 };

    sprite_profile_create(1, rect_base1); // 1: human - male
    sprite_profile_create(1, rect_base2); // 2: human - female
    sprite_profile_create(1, rect_base3); // 3: elf - male
    sprite_profile_create(1, rect_base4); // 4: elf - female
    sprite_profile_create(1, rect_base5); // 5: dwarf - male
    sprite_profile_create(1, rect_base6); // 6: dwarf - female

    u32 rect_hair1[4] = {  0, 0, 32, 32 };
    u32 rect_hair2[4] = { 32, 0, 32, 32 };
    u32 rect_hair3[4] = { 64, 0, 32, 32 };
    u32 rect_hair4[4] = { 96, 0, 32, 32 };

    sprite_profile_create(2, rect_hair1); // 7: male hair
    sprite_profile_create(2, rect_hair2); // 8: male hair
    sprite_profile_create(2, rect_hair3); // 9: female hair
    sprite_profile_create(2, rect_hair4); // 10: female hair

    // PRNG
    std::mt19937 rng(1337);
    std::uniform_real_distribution<f32> dist_x(NPC_MIN_X, NPC_MAX_X);
    std::uniform_real_distribution<f32> dist_y(NPC_MIN_Y, NPC_MAX_Y);
    std::uniform_real_distribution<f32> dist_speed(0.0f, 1.0f);
    std::uniform_real_distribution<f32> dist_angle(0.0f, 6.283185307179586f); // 2 * PI (math.TAU)
    std::uniform_int_distribution<u32> dist_u32;

    // scene
    for (u32 i = 0; i < NPC_COUNT; ++i) {
        u32 ett_key = 0;
        entity *ett = nullptr;
        entity_create(nullptr, nullptr, 0, &ett_key, &ett);

        ett->pos[0] = dist_x(rng);
        ett->pos[1] = dist_y(rng);

        u32 race = dist_u32(rng) % 3;
        u32 gender = dist_u32(rng) % 2;
        u32 profile_idx = race * 2 + gender + 1;

        f32 sprite_offset[2] = { -16.0f, -16.0f };
        entity_new_sprite(ett->pool_key, profile_idx, sprite_offset, 0, nullptr, nullptr, nullptr);

        f32 col_size[2] = { 32.0f, 32.0f };
        f32 col_offset[2] = { -16.0f, -16.0f };
        entity_new_collider(ett->pool_key, col_size, col_offset, 0, nullptr, nullptr);

        f32 speed = dist_speed(rng);
        f32 angle = dist_angle(rng);
        ett->vel[0] = std::cos(angle) * speed;
        ett->vel[1] = std::sin(angle) * speed;

        u32 hair_profile_idx = gender * 2 + 1 + 6 + (dist_u32(rng) % 2);
        f32 hair_offset[2] = {
            -16.0f + HAIR_OFFSET_BY_RACES[race][0],
            -16.0f + HAIR_OFFSET_BY_RACES[race][1]
        };
        entity_new_sprite(ett->pool_key, hair_profile_idx, hair_offset, 1, nullptr, nullptr, nullptr);
    }
}

void game_input() {}
void game_update() {}
void game_update_late() {}

void game_visual() {
    log_trace("fps: %f", 1.0 / time_delta_sec);
}

void game_draw() {
    // _collider_debug_draw();
}

}
