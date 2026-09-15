#include "game.h"

#include "context.h"
#include "draw.h"
#include "entity.h"
#include <raylib.h>
#include <stdlib.h>
#include <math.h>

constexpr usize NPC_COUNT = 5000;
constexpr f32 NPC_MIN_X = -3000;
constexpr f32 NPC_MAX_X =  3000;
constexpr f32 NPC_MIN_Y = -3000;
constexpr f32 NPC_MAX_Y =  3000;

enum {
    RACE_HUMAN,
    RACE_ELF,
    RACE_DWARD,
}; // get race's portrait profile by "$enum * 2 + [female]?"

constexpr Vector2 HAIR_OFFSET_BY_RACES[10] = { {0, 0}, {0, 0}, {0, 3} };

void game_init() {
    // engine set up
    draw_sys_init(1e2, 1e5);

    portrait_sys_init(1e3, 3 * 1e5);
    collider_sys_init(1e5);
    status_sys_init(1e5);
    entity_sys_init(1e5);

    action_sys_init(1e5);

    chunk_sys_init(1e5);

    // texture load
    texture_load("img/char_base.png"); // 1
    texture_load("img/char_hair.png"); // 2

    // portrait profile load
    portrait_profile_create(1,   0, 0, 32, 32); // 1: human - male
    portrait_profile_create(1,  32, 0, 32, 32); // 2: human - female
    portrait_profile_create(1,  64, 0, 32, 32); // 3: elf - male
    portrait_profile_create(1,  96, 0, 32, 32); // 4: elf - female
    portrait_profile_create(1, 128, 0, 32, 32); // 5: dwarf - male
    portrait_profile_create(1, 160, 0, 32, 32); // 6: dwarf - female
    // +6
    portrait_profile_create(2,   0, 0, 32, 32); // 1: male hair
    portrait_profile_create(2,  32, 0, 32, 32); // 2: male hair
    portrait_profile_create(2,  64, 0, 32, 32); // 3: female hair
    portrait_profile_create(2,  96, 0, 32, 32); // 4: female hair

    {
        entity *ett;
        u32 ett_idx = entity_create(0, -1, &ett);

        portrait *potr;
        entity_add_portrait(ett_idx, 5, &potr);
        potr->offset_x = -16;
        potr->offset_y = -16;
        portrait *hair_potr;

        entity_add_portrait(ett_idx, 1 + 6, &hair_potr);
        hair_potr->offset_x = -16 + HAIR_OFFSET_BY_RACES[2].x;
        hair_potr->offset_y = -16 + HAIR_OFFSET_BY_RACES[2].y;
        hair_potr->z_in_entity = 1;
    }
    {
        entity *ett;
        u32 ett_idx = entity_create(10, 0, &ett);
        
        portrait *potr;
        entity_add_portrait(ett_idx, 1, &potr);
        potr->offset_x = -16;
        potr->offset_y = -16;

        portrait *hair_potr;
        entity_add_portrait(ett_idx, 1 + 6, &hair_potr);
        hair_potr->offset_x = -16 + HAIR_OFFSET_BY_RACES[0].x;
        hair_potr->offset_y = -16 + HAIR_OFFSET_BY_RACES[0].y;
        hair_potr->z_in_entity = 1;
    }

    // spawn npcs
    for (usize i = 0; i < NPC_COUNT; ++i) {
        f32 x = NPC_MIN_X + (f32)rand() / (f32)RAND_MAX * (NPC_MAX_X - NPC_MIN_X);
        f32 y = NPC_MIN_Y + (f32)rand() / (f32)RAND_MAX * (NPC_MAX_Y - NPC_MIN_Y);

        u32 race = rand() % 3;
        u32 gender = rand() % 2;
        u32 profile_idx = race * 2 + gender + 1;

        entity *ett;
        u32 ett_idx = entity_create(x, y, &ett);

        portrait *potr;
        entity_add_portrait(ett_idx, profile_idx, &potr);
        potr->offset_x = -16;
        potr->offset_y = -16;

        collider *col;
        entity_add_collider(ett_idx, 32, 32, &col);
        col->offset_x = -16;
        col->offset_y = -16;

        // random scale
        ett->scale_x = ett->scale_y = (f32)rand() / (f32)RAND_MAX * 1.0f + 0.5f;

        // random velocity
        f32 speed = (f32)rand() / (f32)RAND_MAX * 30.0f;
        f32 angle = (f32)rand() / (f32)RAND_MAX * 6.2831853f; // 2*PI
        f32 vx = cosf(angle) * speed * 0;
        f32 vy = sinf(angle) * speed * 0;
        entity_pos_set_velocity(ett_idx, vx, vy);

        // addon
        u32 hair_profile_idx = gender * 2 + 1 + 6 + rand() % 2;
        portrait *hair_potr;
        entity_add_portrait(ett_idx, hair_profile_idx, &hair_potr);
        hair_potr->offset_x = -16 + HAIR_OFFSET_BY_RACES[race].x;
        hair_potr->offset_y = -16 + HAIR_OFFSET_BY_RACES[race].y;
        hair_potr->z_in_entity = 1;
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
