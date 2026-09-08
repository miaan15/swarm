#include "context.h"

#include "entity.h"
#include "draw.h"
#include "sprite.h"

arena omni_arena = {0};

f32 spr_dir[4];

void engine_init() {
    arena_init(&omni_arena, 100 << 10 << 10); // 100MB

    draw_init(1024, 4086);
    sprite_sys_init(256, 1024);
    entity_sys_init(2048);

    texture_load("img/char_00.png");
    sprite_prf_make(1, 0,  0, 20, 20);
    sprite_prf_make(1, 0, 20, 20, 20);
    sprite_prf_make(1, 0, 40, 20, 20);

    sprite_create(1);
    sprite_create(2);
    sprite_create(3);

    sprite *spr = sprite_get(1);
    spr->x = 100; spr->y = 100;

    spr = sprite_get(2);
    spr->x = 105; spr->y = 100;
    spr_dir[2] = 1;

    spr = sprite_get(3);
    spr->x = 100; spr->y = 110;
    spr_dir[3] = 1;
}

void engine_update() {
    sprite *spr = sprite_get(2);
    spr->y += spr_dir[2] * .15f;
    if (spr->y > 130) { spr_dir[2] = -1; spr->y = 110; }
    if (spr->y < 80)  { spr_dir[2] =  1; spr->y = 90;  }

    spr = sprite_get(3);
    spr->x += spr_dir[2] * .1f;
    if (spr->x > 110) { spr_dir[2] = -1; spr->x = 110; }
    if (spr->x < 90)  { spr_dir[2] =  1; spr->x = 90;  }
}

void engine_input() {

}

void _draw_sprite(u32 idx) {
    sprite *spr = sprite_get(idx);
    sprite_prf spr_prf = sprite_prf_get(spr->prf_idx);
    drawer *drr = draw_make();
    drr->tex = spr_prf.tex;
    drr->sx = spr_prf.x;
    drr->sy = spr_prf.y;
    drr->sw = spr_prf.w;
    drr->sh = spr_prf.h;
    drr->dx = spr->x * 4;
    drr->dy = spr->y * 4;
    drr->dw = spr_prf.w * 4;
    drr->dh = spr_prf.h * 4;
    draw_meta_set_y(&drr->meta, drr->dy);
}
void engine_draw() {
    _draw_sprite(1);
    _draw_sprite(2);
    _draw_sprite(3);

    draw_present();
}

void engine_destroy() {
    sprite_destroy(1);
    sprite_destroy(2);
    sprite_destroy(3);
}
