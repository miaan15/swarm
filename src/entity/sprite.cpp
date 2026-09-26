module;

#include <cmath>
#include <cstddef>

export module entity:sprite;
//
// import def;
// import mem;
// import log;
//
// import pool;
//
// import context;
//
// export namespace sw {
//
// struct sprite_profile {
//     u32 tex;
//     u32 rect[4];
// };
//
// struct sprite {
//     u32 key;
//
//     u32 tex;
//     f32 src[4];
//     f32 dest[4];
//     u64 sorting;
//
//     f32 last_tick_pos[2];
//     f32 interpolate_pos[2];
//
//     // chunk
//     u32 chunk_key;
//
//     // entity
//     u32 ett_owner;
//     u32 ett_links[2];
//
//     f32 ett_offset[2];
//     i8 ett_z;
//     f32 ett_scale[2];
// };
//
// struct {
//     sprite_profile *profile_list_ptr;
//     u32 profile_cap;
//     u32 profile_len;
//
//     pool<sprite> sprite_pool;
//     chunk_mng sprite_chunk;
// } sprite_sys = {};
//
// // ================================================================================================
//
// void sprite_sys_init(u32 profile_cap, u32 sprite_cap) {
//     sprite_sys.profile_list_ptr = (sprite_profile*)arena_alloc(&omni_arena, profile_cap * sizeof(sprite_profile));
//     sprite_sys.profile_cap = profile_cap;
//
//     // stub
//     sprite_sys.profile_len = 1;
//     sprite_sys.profile_list_ptr[0] = sprite_profile{ 0, { 0, 0, 32, 32 } };
//
//     pool_init(&sprite_sys.sprite_pool, sprite_cap, offsetof(sprite, key));
//     chunk_mng_init(&sprite_sys.sprite_chunk, 1024, sprite_cap);
// }
//
// // ================================================================================================
//
// u32 sprite_profile_create(u32 tex, const u32 rect[4]) {
//     if (sprite_sys.profile_len >= sprite_sys.profile_cap) {
//         log_err("sprite_profile_create: too much sprite profiles (%u) => stub", sprite_sys.profile_len);
//         return 0;
//     }
//
//     u32 idx = sprite_sys.profile_len;
//     sprite_sys.profile_list_ptr[idx] = sprite_profile{ tex, { rect[0], rect[1], rect[2], rect[3] } };
//     sprite_sys.profile_len++;
//
//     log_debug("created sprite profile [%u]: texture = [%u], rect = (%u, %u, %u, %u)", idx, tex, rect[0], rect[1], rect[2], rect[3]);
//
//     return idx;
// }
//
// sprite_profile sprite_profile_get(u32 idx) {
//     if (idx == 0 || idx >= sprite_sys.profile_len) {
//         log_err("sprite_profile_get: profile [%u] invalid => stub", idx);
//         return sprite_sys.profile_list_ptr[0];
//     }
//     return sprite_sys.profile_list_ptr[idx];
// }
//
// // ================================================================================================
//
// void sprite_create(u32 profile_idx, const f32 dest[4], u64 sorting, u32 *out_key, sprite **out_ptr) {
//     if (sprite_sys.sprite_pool.data_list_len >= sprite_sys.sprite_pool.cap) {
//         log_err("sprite_create: too many sprite (%u) => stub", sprite_sys.sprite_pool.data_list_len);
//         if (out_key) { *out_key = 0; }
//         if (out_ptr) { *out_ptr = &sprite_sys.sprite_pool.data_list_ptr[0]; }
//         return;
//     }
//
//     u32 key = 0;
//     sprite *ptr = nullptr;
//     pool_create(&sprite_sys.sprite_pool, &key, &ptr);
//
//     sprite_profile profile = sprite_profile_get(profile_idx);
//     ptr->tex = profile.tex;
//     ptr->src[0] = (f32)profile.rect[0];
//     ptr->src[1] = (f32)profile.rect[1];
//     ptr->src[2] = (f32)profile.rect[2];
//     ptr->src[3] = (f32)profile.rect[3];
//
//     if (dest) {
//         ptr->dest[0] = dest[0];
//         ptr->dest[1] = dest[1];
//         ptr->dest[2] = dest[2];
//         ptr->dest[3] = dest[3];
//     } else {
//         ptr->dest[0] = 0.0f;
//         ptr->dest[1] = 0.0f;
//         ptr->dest[2] = 0.0f;
//         ptr->dest[3] = 0.0f;
//     }
//
//     ptr->sorting = sorting;
//     ptr->last_tick_pos[0] = NAN;
//     ptr->last_tick_pos[1] = NAN;
//     ptr->interpolate_pos[0] = ptr->dest[0];
//     ptr->interpolate_pos[1] = ptr->dest[1];
//
//     f32 chunk_rect[4] = { ptr->interpolate_pos[0], ptr->interpolate_pos[1], ptr->dest[2], ptr->dest[3] };
//     chunk_mng_create(&sprite_sys.sprite_chunk, key, chunk_cal_center_rect(chunk_rect), &ptr->chunk_key, nullptr);
//
//     log_debug("created sprite [%u]: profile = [%u]; dest = (%.1f, %.1f, %.1f, %.1f); sort = %llu",
//               key, profile_idx, ptr->dest[0], ptr->dest[1], ptr->dest[2], ptr->dest[3], (unsigned long long)sorting);
//
//     if (out_key) { *out_key = key; }
//     if (out_ptr) { *out_ptr = ptr; }
// }
//
// void sprite_destroy(u32 key) {
//     if (!pool_alive(&sprite_sys.sprite_pool, key)) {
//         log_err("sprite_destroy: sprite [%u] invalid (dead or worse)", key);
//         return;
//     }
//
//     sprite *ptr = pool_get(&sprite_sys.sprite_pool, key);
//
//     chunk_mng_destroy(&sprite_sys.sprite_chunk, ptr->chunk_key);
//     pool_destroy(&sprite_sys.sprite_pool, key);
//
//     log_debug("destroyed sprite [%u]", key);
// }
//
// sprite *sprite_get(u32 key) {
//     if (!pool_alive(&sprite_sys.sprite_pool, key)) {
//         log_err("sprite_get: sprite [%u] invalid (dead or worse) => stub", key);
//         return &sprite_sys.sprite_pool.data_list_ptr[0];
//     }
//
//     return pool_get(&sprite_sys.sprite_pool, key);
// }
//
// bool sprite_alive(u32 key) {
//     return pool_alive(&sprite_sys.sprite_pool, key);
// }
//
// // ================================================================================================
//
// void sprite_sys_update_early() {
//     u32 iter_idx = 0;
//     u32 key = 0;
//     sprite *ptr = nullptr;
//
//     while (pool_iterate(&sprite_sys.sprite_pool, &iter_idx, &key, &ptr)) {
//         if (!std::isnan(ptr->last_tick_pos[0]) && !std::isnan(ptr->last_tick_pos[1])) {
//             ptr->last_tick_pos[0] = ptr->dest[0];
//             ptr->last_tick_pos[1] = ptr->dest[1];
//         }
//     }
// }
//
// void sprite_sys_draw() {
//     u32 iter_idx = 0;
//     u32 key = 0;
//     sprite *ptr = nullptr;
//
//     while (pool_iterate(&sprite_sys.sprite_pool, &iter_idx, &key, &ptr)) {
//         f32 pos[2] = { ptr->dest[0], ptr->dest[1] };
//
//         if (!std::isnan(ptr->last_tick_pos[0]) && !std::isnan(ptr->last_tick_pos[1])) {
//             ptr->interpolate_pos[0] = ptr->last_tick_pos[0] + (pos[0] - ptr->last_tick_pos[0]) * (f32)tick_frame_alpha;
//             ptr->interpolate_pos[1] = ptr->last_tick_pos[1] + (pos[1] - ptr->last_tick_pos[1]) * (f32)tick_frame_alpha;
//         } else {
//             ptr->interpolate_pos[0] = pos[0];
//             ptr->interpolate_pos[1] = pos[1];
//             ptr->last_tick_pos[0] = pos[0];
//             ptr->last_tick_pos[1] = pos[1];
//         }
//
//         f32 chunk_rect[4] = { ptr->interpolate_pos[0], ptr->interpolate_pos[1], ptr->dest[2], ptr->dest[3] };
//         chunk_mng_update(&sprite_sys.sprite_chunk, key, chunk_cal_center_rect(chunk_rect));
//
//         draw *drw = draw_new();
//         if (drw) {
//             drw->type = draw_type::TEXTURE;
//             drw->texture.texture_idx = ptr->tex;
//             drw->texture.src_rect[0] = ptr->src[0];
//             drw->texture.src_rect[1] = ptr->src[1];
//             drw->texture.src_rect[2] = ptr->src[2];
//             drw->texture.src_rect[3] = ptr->src[3];
//             drw->texture.dest_rect[0] = ptr->interpolate_pos[0];
//             drw->texture.dest_rect[1] = ptr->interpolate_pos[1];
//             drw->texture.dest_rect[2] = ptr->dest[2];
//             drw->texture.dest_rect[3] = ptr->dest[3];
//             drw->sorting = ptr->sorting;
//         }
//     }
// }
//
// }
