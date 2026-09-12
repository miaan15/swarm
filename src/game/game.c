#include "game.h"
#include "log.h"
#include "entity.h"
#include "draw.h"

void npc_create(Vector2 pos) {
    entity *ett;
    u32 ett_idx = entity_create(&ett);

    u32 spr_prf_idx = 1;
    sprite_profile spr_prf = sprite_profile_get(spr_prf_idx);

    sprite* spr;
    entity_add_sprite(ett_idx, spr_prf_idx, &spr);
    spr->offset_x = -spr_prf.w / 2;
    spr->offset_y = -spr_prf.h / 2;

    collider *col;
    entity_add_collider(ett_idx, &col);
    col->offset_x = -spr_prf.w / 2;
    col->offset_y = -spr_prf.h / 2;

    ett->x = pos.x;
    ett->y = pos.y;

    log_info("Created NPC as Entity [%u]", ett_idx);
}

void game_init() {
    u32 char_tex = texture_load("img/char_00.png");
    sprite_profile_create(char_tex, 0, 0, 20, 20); // 1

    npc_create((Vector2){0, 0});
    npc_create((Vector2){29, 43});
}

void game_update() { }

void game_update_late() { }

void game_input() { }

void game_visual() { }

void game_draw() { }

void game_destroy() { }
