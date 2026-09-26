// classic pool kind of data structure
// - o(1) insertion, deletion, lookup
// - pointer stability
// - using "key", an u32, as direct index handle to read/modify data

module;

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

export module pool_simple;

import def;
import mem;
import context;
import log;

export namespace sw {

constexpr usize _POOL_SIMPLE_KEY_FIELD_OF_T_OFFS_DISABLE_VALUE = (usize)-1;
constexpr u32 _POOL_SIMPLE_ALIVE_FLAG = (u32)-1;

template <typename T>
struct pool_simple {
    u32 *slot_flag_ptr;
    T *data_list_ptr;
    // slot_flag either:
    // - POOL_SIMPLE_ALIVE_FLAG if that slot is alive
    // - store the index that point to next member in slot_flag's free-list, or that slot is dead
    // - slot = 0 means stub / uninitialized

    u32 cap;
    u32 head_key;
    u32 max_key;
    u32 len;

    usize key_field_of_T_offs;
};

// ================================================================================================

/**
 * @param p pool_simple pointer
 * @param cap max capacity
 * @param key_field_of_T_offs byte offset of u32 key inside struct T so pool can automatically set its value (leave default if not has that)
 */
template <typename T>
void pool_simple_init(pool_simple<T> *p, u32 cap, usize key_field_of_T_offs = _POOL_SIMPLE_KEY_FIELD_OF_T_OFFS_DISABLE_VALUE) {
    p->key_field_of_T_offs = key_field_of_T_offs;

    p->slot_flag_ptr = (u32*)arena_alloc(&omni_arena, cap * sizeof(u32));
    p->data_list_ptr = (T*)arena_alloc(&omni_arena, cap * sizeof(T));
    p->cap = cap;

    // stub
    p->slot_flag_ptr[0] = 0;
    memset(&p->data_list_ptr[0], 0, sizeof(T));

    p->head_key = 1;
    p->max_key = 1;
    p->len = 1;
}

// ================================================================================================

/**
 * @brief create a new instance in the pool
 * @param p pool pointer
 * @param out_key out param: key handle
 * @param out_ptr out param: pointer to instance's data
 */
template <typename T>
void pool_simple_create(pool_simple<T> *p, u32 *out_key, T **out_ptr) {
    // return a new instance that was dead, now alive
    // : find the index of a dead slot (the key) which is .head_key,
    //   update len, update slot_flag's free-list, then return that key and ptr to that data

    // pool instances overflow
    if (p->len >= p->cap) {
        log_trace("pool_simple_create: too much instances (%u)", p->len);
        if (out_key) { *out_key = 0; }
        if (out_ptr) { *out_ptr = &p->data_list_ptr[0]; }
        return;
    }

    // get the desired key from .head_key
    // remove the free-list head by update .head_key
    u32 key = p->head_key;
    assert(key <= p->max_key);

    if (key == p->max_key) {
        p->max_key++;
        p->head_key++;
    } else {
        p->head_key = p->slot_flag_ptr[key];
    }

    p->slot_flag_ptr[key] = _POOL_SIMPLE_ALIVE_FLAG;
    p->len += 1;

    T *ptr = &p->data_list_ptr[key];
    memset(ptr, 0, sizeof(T));

    // update "key" field of T if existed
    if (p->key_field_of_T_offs != _POOL_SIMPLE_KEY_FIELD_OF_T_OFFS_DISABLE_VALUE) {
        memcpy((char*)ptr + p->key_field_of_T_offs, &key, sizeof(u32));
    }

    if (out_key) { *out_key = key; }
    if (out_ptr) { *out_ptr = ptr; }
}

/**
 * @brief destroy an instance by key
 * @param p pool pointer
 * @param key key handle to destroy
 * @return true if instance was found and killed, false otherwise
 */
template <typename T>
bool pool_simple_destroy(pool_simple<T> *p, u32 key) {
    // destroy an instance by key
    // : from key, verify slot is alive, push key back to free-list head, update len

    if (key == 0 || key >= p->max_key) {
        log_trace("pool_simple_destroy: [%u] invalid", key);
        return false;
    }

    // slot is already in free-list or instance already dead
    if (p->slot_flag_ptr[key] != _POOL_SIMPLE_ALIVE_FLAG) {
        log_trace("pool_simple_destroy: [%u] already dead", key);
        return false;
    }

    // push key back to free-list head
    p->slot_flag_ptr[key] = p->head_key;
    p->head_key = key;
    p->len -= 1;

    return true;
}

/**
 * @param p pool pointer
 * @param key key handle
 * @return pointer to instance, or stub pointer if invalid/dead
 */
template <typename T>
T *pool_simple_get(pool_simple<T> *p, u32 key) {
    if (key == 0 || key >= p->max_key) {
        log_trace("pool_simple_get: [%u] invalid => stub", key);
        return &p->data_list_ptr[0];
    }

    if (p->slot_flag_ptr[key] != _POOL_SIMPLE_ALIVE_FLAG) {
        log_trace("pool_simple_get: [%u] is dead => stub", key);
        return &p->data_list_ptr[0];
    }

    return &p->data_list_ptr[key];
}

/**
 * @param p pool pointer
 * @param key key handle
 * @return true if alive, or false if invalid/dead
 */
template <typename T>
bool pool_simple_alive(pool_simple<T> *p, u32 key) {
    if (key == 0 || key >= p->max_key) {
        log_trace("pool_simple_alive: [%u] invalid", key);
        return false;
    }

    return p->slot_flag_ptr[key] == _POOL_SIMPLE_ALIVE_FLAG;
}

/**
 * @param p pool pointer
 * @param iter_idx state index pointer (pass 0 to start)
 * @param out_key out param: instance key
 * @param out_ptr out param: pointer to instance data
 * @return true if instance found, false when finished
 */
template <typename T>
bool pool_simple_iterate(pool_simple<T> *p, u32 *iter_idx, u32 *out_key, T **out_ptr) {
    if (*iter_idx == 0) {
        *iter_idx = 1;
    }

    while (*iter_idx < p->max_key) {
        u32 key = *iter_idx;
        *iter_idx += 1;

        if (p->slot_flag_ptr[key] == _POOL_SIMPLE_ALIVE_FLAG) {
            T *ptr = &p->data_list_ptr[key];

            if (out_ptr) { *out_ptr = ptr; }
            if (out_key) { *out_key = key; }

            return true;
        }
    }

    return false;
}

// ================================================================================================
// DEBUG
// ================================================================================================

template <typename T>
bool _pool_simple_validate(pool_simple<T> *p) {
    if (p->len > p->cap || p->max_key > p->cap || p->len == 0) {
        log_trace("pool_simple validate: invalid bounds (len=%u, cap=%u, max_key=%u)", p->len, p->cap, p->max_key);
        return false;
    }

    u32 cnt_alive = 0;
    for (u32 key = 1; key < p->max_key; ++key) {
        if (!pool_simple_alive(p, key)) {
            continue;
        }

        cnt_alive += 1;

        if (p->key_field_of_T_offs != _POOL_SIMPLE_KEY_FIELD_OF_T_OFFS_DISABLE_VALUE) {
            u32 data_key = 0;
            memcpy(&data_key, reinterpret_cast<char*>(&p->data_list_ptr[key]) + p->key_field_of_T_offs, sizeof(u32));
            if (data_key != key) {
                log_trace("pool_simple validate: key mismatch at key %u (found %u)", key, data_key);
                return false;
            }
        }
    }

    if (cnt_alive != p->len - 1) {
        log_trace("pool_simple validate: alive count = %u, expected %u", cnt_alive, p->len - 1);
        return false;
    }

    bool *visited = static_cast<bool*>(calloc(p->max_key + 1, sizeof(bool)));
    if (!visited) {
        return false;
    }

    u32 curr = p->head_key;
    while (curr != 0 && curr < p->max_key) {
        if (visited[curr]) {
            log_trace("pool_simple validate: cycle detected in free list at key [%u]", curr);
            free(visited);
            return false;
        }
        visited[curr] = true;

        if (p->slot_flag_ptr[curr] == _POOL_SIMPLE_ALIVE_FLAG) {
            log_trace("pool_simple validate: active slot found inside free list at key [%u]", curr);
            free(visited);
            return false;
        }
        curr = p->slot_flag_ptr[curr];
    }

    free(visited);
    return true;
}

template <typename T>
void _pool_simple_debug_log(pool_simple<T> *p) {
    usize buf_sz = 512 + static_cast<usize>(p->max_key) * 16;
    char *buf = static_cast<char*>(malloc(buf_sz));
    if (!buf) {
        return;
    }

    usize offset = 0;
    offset += snprintf(
        buf + offset,
        buf_sz - offset,
        "--- POOL_SIMPLE DEBUG: len: %u; max_key: %u; cap: %u; head: %u ---\nSTATUS: %s\n",
        p->len, p->max_key, p->cap, p->head_key,
        _pool_simple_validate(p) ? "ok" : "ERROR"
    );

    // Keys row
    offset += snprintf(buf + offset, buf_sz - offset, "KEYS: ");
    for (u32 key = 0; key < p->max_key; ++key) {
        offset += snprintf(buf + offset, buf_sz - offset, " %2u  ", key);
    }
    offset += snprintf(buf + offset, buf_sz - offset, "\n");

    // Active row
    offset += snprintf(buf + offset, buf_sz - offset, "ACTV: ");
    for (u32 key = 0; key < p->max_key; ++key) {
        if (pool_simple_alive(p, key)) {
            offset += snprintf(buf + offset, buf_sz - offset, "[ALV] ");
        } else {
            offset += snprintf(buf + offset, buf_sz - offset, "[   ] ");
        }
    }
    offset += snprintf(buf + offset, buf_sz - offset, "\n");

    // Free row
    offset += snprintf(buf + offset, buf_sz - offset, "FREE: ");
    for (u32 key = 0; key < p->max_key; ++key) {
        if (!pool_simple_alive(p, key)) {
            offset += snprintf(buf + offset, buf_sz - offset, "[%2u] ", p->slot_flag_ptr[key]);
        } else {
            offset += snprintf(buf + offset, buf_sz - offset, "[   ] ");
        }
    }
    offset += snprintf(buf + offset, buf_sz - offset, "\n");

    log_info("%s", buf);
    free(buf);
}

}
