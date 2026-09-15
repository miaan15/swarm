#include "chunk.h"
#include "context.h"
#include "log.h"
#include <math.h>

struct chunk_sys chunk_sys = {0};

// =============================================================================
usize chunk_hash(i32 cx, i32 cy);
usize chunk_find(i32 x, i32 y);
usize chunk_open(f32 x, f32 y);

// =============================================================================
void chunk_sys_init(usize cap) {
    chunk_sys.chunk_map = arena_alloc(&omni_arena, cap * sizeof(chunk_entry));
    chunk_sys.chunk_cap = cap;

    // stub
    chunk_sys.chunk_len = 1;
}

// =============================================================================
void chunk_cal_pos(f32 x, f32 y, i32 *cx, i32 *cy) {
    if (cx != nullptr) *cx = floorf(x / CHUNK_SIZE);
    if (cy != nullptr) *cy = floorf(y / CHUNK_SIZE);
}

// =============================================================================
void chunk_add_entity(u32 ett_idx) {
    if (ett_idx == 0 || ett_idx > entity_sys.entity_max_idx) {
        log_err("chunk_add_entity(): Entity [%u] is invalid", ett_idx);
        return;
    }

    f32 x, y;
    entity_pos_get(ett_idx, &x, &y, 0, 0, 0, 0);
    entity *ett = entity_get(ett_idx);

    usize idx = chunk_open(x, y);
    chunk_entry *chunk = &chunk_sys.chunk_map[idx];

    // linking
    ett->next_in_chunk = chunk->entity_begin;
    ett->pre_in_chunk = 0;

    if (chunk->entity_begin != 0) {
        entity_get(chunk->entity_begin)->pre_in_chunk = ett_idx;
    }

    chunk->entity_begin = ett_idx;

    // set
    chunk_cal_pos(x, y, &ett->chunk_x, &ett->chunk_y);
}

void chunk_add_portrait(u32 potr_idx) {
    if (potr_idx == 0 || potr_idx > portrait_sys.portrait_max_idx) {
        log_err("chunk_add_portrait(): Portrait [%u] is invalid", potr_idx);
        return;
    }

    portrait *potr = portrait_get(potr_idx);

    usize idx = chunk_open(potr->x + (potr->w / 2), potr->y + (potr->h / 2));
    chunk_entry *chunk = &chunk_sys.chunk_map[idx];

    // linking
    potr->next_in_chunk = chunk->portrait_begin;
    potr->pre_in_chunk = 0;

    if (chunk->portrait_begin != 0) {
        portrait_get(chunk->portrait_begin)->pre_in_chunk = potr_idx;
    }

    chunk->portrait_begin = potr_idx;

    // set
    chunk_cal_pos(potr->x + (potr->w / 2), potr->y + (potr->h / 2), &potr->chunk_x, &potr->chunk_y);
}

void chunk_add_collider(u32 col_idx) {
    if (col_idx == 0 || col_idx > collider_sys.collider_max_idx) {
        log_err("chunk_add_collider(): Collider [%u] is invalid", col_idx);
        return;
    }

    collider *col = collider_get(col_idx);

    usize idx = chunk_open(col->x + (col->w / 2), col->y + (col->h / 2));
    chunk_entry *chunk = &chunk_sys.chunk_map[idx];

    // linking
    col->next_in_chunk = chunk->collider_begin;
    col->pre_in_chunk = 0;

    if (chunk->collider_begin != 0) {
        collider_get(chunk->collider_begin)->pre_in_chunk = col_idx;
    }

    chunk->collider_begin = col_idx;

    // set
    chunk_cal_pos(col->x + (col->w / 2), col->y + (col->h / 2), &col->chunk_x, &col->chunk_y);
}

// =============================================================================
void chunk_remv_entity(u32 ett_idx) {
    if (ett_idx == 0 || ett_idx > entity_sys.entity_max_idx) {
        log_err("chunk_remv_entity(): Entity [%u] is invalid", ett_idx);
        return;
    }

    entity *ett = entity_get(ett_idx);

    usize idx = chunk_find(ett->chunk_x, ett->chunk_y);
    chunk_entry *chunk = &chunk_sys.chunk_map[idx];

    // link pre
    if (ett->pre_in_chunk != 0) {
        entity_get(ett->pre_in_chunk)->next_in_chunk = ett->next_in_chunk;
    } else if (chunk->entity_begin == ett_idx) {
        chunk->entity_begin = ett->next_in_chunk;
    }

    // link next
    if (ett->next_in_chunk != 0) {
        entity_get(ett->next_in_chunk)->pre_in_chunk = ett->pre_in_chunk;
    }

    ett->pre_in_chunk = 0;
    ett->next_in_chunk = 0;
}

void chunk_remv_portrait(u32 potr_idx) {
    if (potr_idx == 0 || potr_idx > portrait_sys.portrait_max_idx) {
        log_err("chunk_remv_portrait(): Portrait [%u] is invalid", potr_idx);
        return;
    }

    portrait *potr = portrait_get(potr_idx);

    usize idx = chunk_find(potr->chunk_x, potr->chunk_y);
    chunk_entry *chunk = &chunk_sys.chunk_map[idx];

    // link pre
    if (potr->pre_in_chunk != 0) {
        portrait_get(potr->pre_in_chunk)->next_in_chunk = potr->next_in_chunk;
    } else if (chunk->portrait_begin == potr_idx) {
        chunk->portrait_begin = potr->next_in_chunk;
    }

    // link next
    if (potr->next_in_chunk != 0) {
        portrait_get(potr->next_in_chunk)->pre_in_chunk = potr->pre_in_chunk;
    }

    potr->pre_in_chunk = 0;
    potr->next_in_chunk = 0;
}

void chunk_remv_collider(u32 col_idx) {
    if (col_idx == 0 || col_idx > collider_sys.collider_max_idx) {
        log_err("chunk_remv_collider(): Collider [%u] is invalid", col_idx);
        return;
    }

    collider *col = collider_get(col_idx);

    usize idx = chunk_find(col->chunk_x, col->chunk_y);
    chunk_entry *chunk = &chunk_sys.chunk_map[idx];

    // link pre
    if (col->pre_in_chunk != 0) {
        collider_get(col->pre_in_chunk)->next_in_chunk = col->next_in_chunk;
    } else if (chunk->collider_begin == col_idx) {
        chunk->collider_begin = col->next_in_chunk;
    }

    // link next
    if (col->next_in_chunk != 0) {
        collider_get(col->next_in_chunk)->pre_in_chunk = col->pre_in_chunk;
    }

    col->pre_in_chunk = 0;
    col->next_in_chunk = 0;
}

// =============================================================================
void chunk_update_entity(u32 ett_idx) {
    if (ett_idx == 0 || ett_idx > entity_sys.entity_max_idx) {
        log_err("chunk_update_entity(): Entity [%u] is invalid", ett_idx);
        return;
    }

    entity *ett = entity_get(ett_idx);

    f32 x, y;
    entity_pos_get(ett_idx, &x, &y, 0, 0, 0, 0);

    i32 cx, cy;
    chunk_cal_pos(x, y, &cx, &cy);

    if (cx != ett->chunk_x || cy != ett->chunk_y) {
        chunk_remv_entity(ett_idx);
        chunk_add_entity(ett_idx);
    }
}

void chunk_update_portrait(u32 potr_idx) {
    if (potr_idx == 0 || potr_idx > portrait_sys.portrait_max_idx) {
        log_err("chunk_update_portrait(): Portrait [%u] is invalid", potr_idx);
        return;
    }

    portrait *potr = portrait_get(potr_idx);

    i32 cx, cy;
    chunk_cal_pos(potr->x + (potr->w / 2.0f), potr->y + (potr->h / 2.0f), &cx, &cy);

    if (cx != potr->chunk_x || cy != potr->chunk_y) {
        chunk_remv_portrait(potr_idx);
        chunk_add_portrait(potr_idx);
    }
}

void chunk_update_collider(u32 col_idx) {
    if (col_idx == 0 || col_idx > collider_sys.collider_max_idx) {
        log_err("chunk_update_collider(): Collider [%u] is invalid", col_idx);
        return;
    }

    collider *col = collider_get(col_idx);

    i32 cx, cy;
    chunk_cal_pos(col->x + (col->w / 2.0f), col->y + (col->h / 2.0f), &cx, &cy);

    if (cx != col->chunk_x || cy != col->chunk_y) {
        chunk_remv_collider(col_idx);
        chunk_add_collider(col_idx);
    }
}

// =============================================================================
void chunk_query_entity(f32 x, f32 y, f32 w, f32 h, u32 **ett_list, usize *ett_list_len) {
    if (ett_list == nullptr || ett_list_len == nullptr) return;
    u32 *list = nullptr;
    usize len = 0, cap = 0;

    i32 min_cx, min_cy, max_cx, max_cy;
    chunk_cal_pos(x - ((f32)CHUNK_SIZE / 2), y - ((f32)CHUNK_SIZE / 2), &min_cx, &min_cy);
    chunk_cal_pos(x + w + (f32)CHUNK_SIZE, y + h + (f32)CHUNK_SIZE, &max_cx, &max_cy);

    for (i32 cy = min_cy; cy <= max_cy; ++cy) {
        for (i32 cx = min_cx; cx <= max_cx; ++cx) {
            usize idx = chunk_find(cx, cy);
            if (idx == 0) continue;

            chunk_entry *chunk = &chunk_sys.chunk_map[idx];
            u32 ett_idx = chunk->entity_begin;
            while (ett_idx != 0) {
                entity *ett = entity_get(ett_idx);

                if (len >= cap) {
                    cap = cap < 4 ? 4 : cap * 2;
                    u32 *_new = arena_alloc(tick_arena, cap * sizeof(u32));
                    if (list != nullptr) { memcpy(_new, list, len * sizeof(u32)); }
                    list = _new;
                }
                list[len++] = ett_idx;

                ett_idx = ett->next_in_chunk;
            }
        }
    }

    *ett_list = list;
    *ett_list_len = len;
}

void chunk_query_portrait(f32 x, f32 y, f32 w, f32 h, u32 **potr_list, usize *potr_list_len) {
    if (potr_list == nullptr || potr_list_len == nullptr) return;

    u32 *list = nullptr;
    usize len = 0, cap = 0;
    f32 pad = (f32)CHUNK_SIZE / 2;

    i32 min_cx, min_cy, max_cx, max_cy;
    chunk_cal_pos(x - pad, y - pad, &min_cx, &min_cy);
    chunk_cal_pos(x + w + pad, y + h + pad, &max_cx, &max_cy);

    for (i32 cy = min_cy; cy <= max_cy; ++cy) {
        for (i32 cx = min_cx; cx <= max_cx; ++cx) {
            usize idx = chunk_find(cx, cy);
            if (idx == 0) continue;

            chunk_entry *chunk = &chunk_sys.chunk_map[idx];
            u32 potr_idx = chunk->portrait_begin;

            while (potr_idx != 0) {
                portrait *potr = portrait_get(potr_idx);

                if (len >= cap) {
                    cap = (cap < 4) ? 4 : cap * 2;
                    u32 *_new = arena_alloc(tick_arena, cap * sizeof(u32));
                    if (list != nullptr) {
                        memcpy(_new, list, len * sizeof(u32));
                    }
                    list = _new;
                }

                list[len++] = potr_idx;
                potr_idx = potr->next_in_chunk;
            }
        }
    }

    *potr_list = list;
    *potr_list_len = len;
}

void chunk_query_collider(f32 x, f32 y, f32 w, f32 h, u32 **col_list, usize *col_list_len) {
    if (col_list == nullptr || col_list_len == nullptr) return;

    u32 *list = nullptr;
    usize len = 0, cap = 0;
    f32 pad = (f32)CHUNK_SIZE / 2;

    i32 min_cx, min_cy, max_cx, max_cy;
    chunk_cal_pos(x - pad, y - pad, &min_cx, &min_cy);
    chunk_cal_pos(x + w + pad, y + h + pad, &max_cx, &max_cy);

    for (i32 cy = min_cy; cy <= max_cy; ++cy) {
        for (i32 cx = min_cx; cx <= max_cx; ++cx) {
            usize idx = chunk_find(cx, cy);
            if (idx == 0) continue;

            chunk_entry *chunk = &chunk_sys.chunk_map[idx];
            u32 col_idx = chunk->collider_begin;

            while (col_idx != 0) {
                collider *col = collider_get(col_idx);

                if (len >= cap) {
                    cap = (cap < 4) ? 4 : cap * 2;
                    u32 *_new = arena_alloc(tick_arena, cap * sizeof(u32));
                    if (list != nullptr) {
                        memcpy(_new, list, len * sizeof(u32));
                    }
                    list = _new;
                }

                list[len++] = col_idx;
                col_idx = col->next_in_chunk;
            }
        }
    }

    *col_list = list;
    *col_list_len = len;
}

void chunk_query(f32 x, f32 y, f32 w, f32 h, chunk_entry **entry_list, usize *entry_list_len) {
    if (entry_list == nullptr || entry_list_len == nullptr) return;

    chunk_entry *list = nullptr;
    usize len = 0, cap = 0;
    f32 pad = (f32)CHUNK_SIZE / 2;

    i32 min_cx, min_cy, max_cx, max_cy;
    chunk_cal_pos(x - pad, y - pad, &min_cx, &min_cy);
    chunk_cal_pos(x + w + pad, y + h + pad, &max_cx, &max_cy);

    for (i32 cy = min_cy; cy <= max_cy; ++cy) {
        for (i32 cx = min_cx; cx <= max_cx; ++cx) {
            usize idx = chunk_find(cx, cy);
            if (idx == 0) continue;

            if (len >= cap) {
                cap = (cap < 4) ? 4 : cap * 2;
                chunk_entry *_new = arena_alloc(tick_arena, cap * sizeof(chunk_entry));
                if (list != nullptr) {
                    memcpy(_new, list, len * sizeof(chunk_entry));
                }
                list = _new;
            }

            list[len++] = chunk_sys.chunk_map[idx];
        }
    }

    *entry_list = list;
    *entry_list_len = len;
}

// PRIVATE
// =============================================================================
usize chunk_hash(i32 cx, i32 cy) {
    // some kind of algorithm idk
    u64 hash = ((u64)(u32)cx << 32) | (u64)(u32)cy;
    hash ^= hash >> 30;
    hash *= 0xbf58476d1ce4e5b9ull;
    hash ^= hash >> 27;
    hash *= 0x94d049bb133111ebull;
    hash ^= hash >> 31;

    return (usize)(hash % (chunk_sys.chunk_cap - 1)) + 1; // [1, chunk_sys.chunk_cap)
}

usize chunk_find(i32 cx, i32 cy) {
    usize h = chunk_hash(cx, cy);
    usize i = h;
    do {
        chunk_entry *chunk = &chunk_sys.chunk_map[i];
        if (!chunk->alive) { return 0; }
        if (chunk->cx == cx && chunk->cy == cy) { return i; }

        ++i;
        if (i >= chunk_sys.chunk_cap) { i = 1; }
    } while (i != h);

    return 0;
}

usize chunk_open(f32 x, f32 y) {
    i32 cx, cy;
    chunk_cal_pos(x, y, &cx, &cy);

    usize h = chunk_hash(cx, cy);
    usize i = h;
    do {
        chunk_entry *chunk = &chunk_sys.chunk_map[i];
        if (!chunk->alive || (chunk->cx == cx && chunk->cy == cy)) { break; }

        ++i;
        if (i >= chunk_sys.chunk_cap) { i = 1; }
    } while (i != h);

    chunk_sys.chunk_map[i].alive = true;
    chunk_sys.chunk_map[i].cx = cx;
    chunk_sys.chunk_map[i].cy = cy;

    return i;
}
