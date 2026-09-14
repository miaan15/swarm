#include "game.h"

#include "context.h"
#include "draw.h"
#include "entity.h"
#include <raylib.h>
#include <stdlib.h>

constexpr usize NPC_COUNT = 10000;
constexpr f32 NPC_MIN_X = -2000;
constexpr f32 NPC_MAX_X =  2000;
constexpr f32 NPC_MIN_Y = -2000;
constexpr f32 NPC_MAX_Y =  2000;

enum {
    RACE_HUMAN,
    RACE_ELF,
    RACE_DWARD,
}; // get race's sprite profile by "$enum * 2 + [female]?"

constexpr Vector2 HAIR_OFFSET_BY_RACES[10] = { {0}, {0}, {0, 3} };

void game_init() {
    // arena alloc
    arena_init(&omni_arena, 500 << 10 << 10); // 500MB
    arena_init_in_arena(&tick_arena_raw[0], &omni_arena, 50 << 10 << 10); // 50MB
    arena_init_in_arena(&tick_arena_raw[1], &omni_arena, 50 << 10 << 10); // 50MB
    tick_arena = &tick_arena_raw[cur_tick_arena_idx];

    // engine set up
    draw_sys_init(1e2, 1e5);

    sprite_sys_init(1e5, 1e5);
    collider_sys_init(1e5, 30.0f);
    effect_sys_init(1e5);
    entity_sys_init(1e5);

    action_sys_init(1e5);

    // texture load
    texture_load("img/char_base.png"); // 1
    texture_load("img/char_hair.png"); // 2

    // sprite profile load
    sprite_profile_create(1,   0, 0, 32, 32); // 1: human - male
    sprite_profile_create(1,  32, 0, 32, 32); // 2: human - female
    sprite_profile_create(1,  64, 0, 32, 32); // 3: elf - male
    sprite_profile_create(1,  96, 0, 32, 32); // 4: elf - female
    sprite_profile_create(1, 128, 0, 32, 32); // 5: dwarf - male
    sprite_profile_create(1, 160, 0, 32, 32); // 6: dwarf - female
    // +6
    sprite_profile_create(2,   0, 0, 32, 32); // 1: male hair
    sprite_profile_create(2,  32, 0, 32, 32); // 2: male hair
    sprite_profile_create(2,  64, 0, 32, 32); // 3: female hair
    sprite_profile_create(2,  96, 0, 32, 32); // 4: female hair

    // spawn npcs
    for (usize i = 0; i < NPC_COUNT; ++i) {
        f32 x = NPC_MIN_X + (f32)rand() / (f32)RAND_MAX * (NPC_MAX_X - NPC_MIN_X);
        f32 y = NPC_MIN_Y + (f32)rand() / (f32)RAND_MAX * (NPC_MAX_Y - NPC_MIN_Y);

        u32 race = rand() % 3;
        u32 gender = rand() % 2;
        u32 profile_idx = race * 2 + gender + 1;

        entity *ett;
        u32 ett_idx = entity_create(x, y, &ett);

        sprite *spr;
        entity_add_sprite(ett_idx, profile_idx, &spr);
        spr->offset_x = -16;
        spr->offset_y = -16;

        // add
        u32 hair_profile_idx = gender * 2 + 1 + 6 + rand() % 2;

        sprite *hair_spr;
        entity_add_sprite(ett_idx, hair_profile_idx, &hair_spr);
        hair_spr->offset_x = -16 + HAIR_OFFSET_BY_RACES[race].x;
        hair_spr->offset_y = -16 + HAIR_OFFSET_BY_RACES[race].y;
    }
}

constexpr f32 CAMERA_SPEED = 500.0f;
constexpr f32 ZOOM_SPEED = 1.0f;
constexpr f32 ZOOM_MIN = 0.1f;
constexpr f32 ZOOM_MAX = 4.0f;

void game_input() {
    f32 dt = (f32)time_delta_ms / 1000.0f;
    f32 speed = CAMERA_SPEED * dt / camera_zoom;
    f32 zoom_speed = ZOOM_SPEED * dt;

    if (IsKeyDown(KEY_W)) camera_y -= speed;
    if (IsKeyDown(KEY_S)) camera_y += speed;
    if (IsKeyDown(KEY_A)) camera_x -= speed;
    if (IsKeyDown(KEY_D)) camera_x += speed;

    if (IsKeyDown(KEY_E)) camera_zoom += zoom_speed;
    if (IsKeyDown(KEY_Q)) camera_zoom -= zoom_speed;

    if (camera_zoom < ZOOM_MIN) camera_zoom = ZOOM_MIN;
    if (camera_zoom > ZOOM_MAX) camera_zoom = ZOOM_MAX;
}

void game_update() { }

void game_update_late() { }

void game_visual() { }

void game_draw() { }

void game_destroy() { }
