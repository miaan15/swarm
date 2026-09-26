module;

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

export module entity;

import def;
import mem;
import context;
import log;

export namespace sw {

constexpr usize _KEY_FIELD_OF_T_OFFS_DISABLE_VALUE = (usize)-1;

template <typename T>
struct pool {
    i32 *slot_pool_ptr;
    T *data_list_ptr;
    // slot_pool either:
    // - store the index that point to data_list, which the index to actual data, or that slot is alive if slot < 0 (data_list index = -slot)
    // - store the index that point to next member in slot_pool's free-list, or that slot is dead if slot > 0 (slot_pool index = +slot)
    // - slot = 0 means not reached to that slot yet, or that slot is super dead

    u32 cap;
    u32 head_key;
    u32 slot_pool_len;
    u32 data_list_len;

    usize key_field_of_T_offs;
};

// ================================================================================================

/**
 * @param p pool pointer
 * @param cap max capacity
 * @param key_field_of_T_offs byte offset of u32 key inside struct T so pool can automatically set its value (leave default if not has that)
 */
template <typename T>
void pool_init(pool<T> *p, u32 cap, usize key_field_of_T_offs = _KEY_FIELD_OF_T_OFFS_DISABLE_VALUE) {
    p->key_field_of_T_offs = key_field_of_T_offs;

    p->slot_pool_ptr = (i32*)arena_alloc(&omni_arena, cap * sizeof(i32));
    p->data_list_ptr = (T*)arena_alloc(&omni_arena, cap * sizeof(T));
    p->cap = cap;

    // stub
    p->head_key = 1;
    p->slot_pool_len = 1;
    p->data_list_len = 1;
}

// ================================================================================================

/**
 * @brief create a new instance in the pool
 * @param p pool pointer
 * @param _key out param: key handle
 * @param _ptr out param: pointer to instance's data
 */
template <typename T>
void pool_create(pool<T> *p, u32 *out_key, T **out_ptr) {
    // return a new instance that was dead, now alive
    // : find the index of a dead slot (the key) which is .head_key, append new data to data_list,
    //   update len, update slot_pool's free-list, then return that key and ptr to that data_list

    // pool instances overflow
    if (p->data_list_len >= p->cap) {
        log_trace("pool_create: too much instances (%u)", p->data_list_len);
        if (out_key) { *out_key = 0; }
        if (out_ptr) { *out_ptr = &p->data_list_ptr[0]; }
        return;
    }

    // get the desired key from .head_key
    // remove the free-list head by update .head_key
    u32 key = p->head_key;
    if (key == p->slot_pool_len) {
        p->slot_pool_len++;
        p->head_key++;
    } else {
        p->head_key = p->slot_pool_ptr[key];
    }

    // new instance will be at the end of data_list, its slot_pool will point to that (alive instance slot has to <0)
    p->slot_pool_ptr[key] = -(i32)p->data_list_len;
    assert(p->slot_pool_ptr[key] < 0);

    // append new data to data_list
    T *ptr = &p->data_list_ptr[p->data_list_len];
    p->data_list_len += 1;

    memset(ptr, 0, sizeof(T));

    // update "key" field of T if existed
    if (p->key_field_of_T_offs != _KEY_FIELD_OF_T_OFFS_DISABLE_VALUE) {
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
bool pool_destroy(pool<T> *p, u32 key) {
    // destroy an instance by key
    // : from key, get the slot, get the index in data_list, remove from data_list by swap-and-pop its data with the last element
    //   update len, update slot_pool's free-list (remember to update slot of pre-swap data_list last element)

    if (key == 0 || key >= p->slot_pool_len) {
        log_trace("pool_destroy: [%u] invalid", key);
        return false;
    }

    // slot is already in free-list or instance already dead
    if (p->slot_pool_ptr[key] >= 0) {
        log_trace("pool_destroy: [%u] already dead", key);
        return false;
    }

    // get data_list index from slot (slot is negative when alive)
    u32 idx = (u32)(-p->slot_pool_ptr[key]);

    // push key back to free-list head
    p->slot_pool_ptr[key] = (i32)p->head_key;
    p->head_key = key;

    assert(idx < p->data_list_len);

    // swap-and-pop if not deleting the last element
    if (idx != p->data_list_len - 1) {
        T *deleted_ptr = &p->data_list_ptr[idx];
        T *replaced_ptr = &p->data_list_ptr[p->data_list_len - 1]; // last element

        // get the key of the last element so we can redirect its slot
        u32 repl_key = 0;
        memcpy(&repl_key, (char*)replaced_ptr + p->key_field_of_T_offs, sizeof(u32));
        assert(repl_key < p->slot_pool_len);

        // overwrite deleted slot with last element
        memcpy(deleted_ptr, replaced_ptr, sizeof(T));
        memcpy((char*)deleted_ptr + p->key_field_of_T_offs, &repl_key, sizeof(u32));

        // redirect slot of the moved element to its new packed index
        p->slot_pool_ptr[repl_key] = -(i32)idx;
    }

    p->data_list_len -= 1;

    return true;
}

/**
 * @param p pool pointer
 * @param key key handle
 * @return pointer to instance, or stub pointer if invalid/dead
 */
template <typename T>
T *pool_get(pool<T> *p, u32 key) {
    if (key == 0 || key >= p->slot_pool_len) {
        log_trace("pool_get: [%u] invalid => stub", key);
        return &p->data_list_ptr[0];
    }

    if (p->slot_pool_ptr[key] >= 0) {
        log_trace("pool_get: [%u] is dead => stub", key);
        return &p->data_list_ptr[0];
    }

    i32 idx = -p->slot_pool_ptr[key];
    assert(idx > 0 && (u32)idx < p->data_list_len);
    return &p->data_list_ptr[idx];
}
/**
 * @param p pool pointer
 * @param key key handle
 * @return true if alive, or false if invalid/dead
 */
template <typename T>
bool pool_alive(pool<T> *p, u32 key) {
    if (key == 0 || key >= p->slot_pool_len) {
        log_trace("pool_alive: [%u] invalid", key);
        return false;
    }

    assert(p->slot_pool_ptr[key] > -(i32)p->data_list_len);
    return p->slot_pool_ptr[key] < 0;
}

/**
 * @param p pool pointer
 * @param iter_idx state index pointer (pass 0 to start)
 * @param out_key out param: instance key
 * @param out_ptr out param: pointer to instance data
 * @return true if instance found, false when finished
 */
template <typename T>
bool pool_iterate(pool<T> *p, u32 *iter_idx, u32 *out_key, T **out_ptr) {
    if (*iter_idx == 0) {
        *iter_idx = 1;
    }

    if (*iter_idx < p->data_list_len) {
        T *ptr = &p->data_list_ptr[*iter_idx];

        if (out_ptr) { *out_ptr = ptr; }
        if (out_key) {
            if (p->key_field_of_T_offs != _KEY_FIELD_OF_T_OFFS_DISABLE_VALUE) {
                memcpy(out_key, (char*)ptr + p->key_field_of_T_offs, sizeof(u32));
            }
        }

        *iter_idx += 1;
        return true;
    }

    return false;
}

// ================================================================================================
// DEBUG
// ================================================================================================

template <typename T>
bool _pool_validate(pool<T> *p) {
    if (p->data_list_len > p->cap || p->slot_pool_len > p->cap || p->data_list_len == 0) {
        log_trace("pool validate: invalid bounds (len=%u, cap=%u, max_key=%u)", p->data_list_len, p->cap, p->slot_pool_len);
        return false;
    }

    u32 cnt_alive = 0;
    u32 max_idx = 0;

    for (u32 key = 1; key < p->slot_pool_len; ++key) {
        if (!pool_alive(p, key)) {
            continue;
        }

        cnt_alive += 1;
        u32 idx = static_cast<u32>(-p->slot_pool_ptr[key]);

        if (idx == 0 || idx >= p->data_list_len) {
            log_trace("pool validate: key [%u] slot points out of range (%d)", key, p->slot_pool_ptr[key]);
            return false;
        }

        u32 data_key = 0;
        memcpy(&data_key, reinterpret_cast<char*>(&p->data_list_ptr[idx]) + p->key_field_of_T_offs, sizeof(u32));
        if (data_key != key) {
            log_trace("pool validate: key mismatch at idx %u (expected %u, found %u)", idx, key, data_key);
            return false;
        }

        if (idx > max_idx) {
            max_idx = idx;
        }
    }

    if (cnt_alive != p->data_list_len - 1) {
        log_trace("pool validate: alive count = %u, expected %u", cnt_alive, p->data_list_len - 1);
        return false;
    }

    if (cnt_alive > 0 && max_idx != p->data_list_len - 1) {
        log_trace("pool validate: max packed idx = %u, expected %u", max_idx, p->data_list_len - 1);
        return false;
    }

    bool *visited = static_cast<bool*>(calloc(p->slot_pool_len, sizeof(bool)));
    if (!visited) {
        return false;
    }

    u32 curr = p->head_key;
    while (curr != 0 && curr < p->slot_pool_len) {
        if (visited[curr]) {
            log_trace("pool validate: cycle detected in free list at key [%u]", curr);
            free(visited);
            return false;
        }
        visited[curr] = true;

        i32 next = p->slot_pool_ptr[curr];
        if (next < 0) {
            log_trace("pool validate: active slot found inside free list at key [%u]", curr);
            free(visited);
            return false;
        }
        curr = static_cast<u32>(next);
    }

    free(visited);
    return true;
}

template <typename T>
void _pool_debug_log(pool<T> *p) {
    usize buf_sz = 512 + static_cast<usize>(p->slot_pool_len) * 16;
    char *buf = static_cast<char*>(malloc(buf_sz));
    if (!buf) {
        return;
    }

    usize offset = 0;
    offset += snprintf(
        buf + offset,
        buf_sz - offset,
        "--- POOL DEBUG: len: %u; max_key: %u; cap: %u; head: %u ---\nSTATUS: %s\n",
        p->data_list_len, p->slot_pool_len, p->cap, p->head_key,
        _pool_validate(p) ? "ok" : "ERROR"
    );

    // Keys row
    offset += snprintf(buf + offset, buf_sz - offset, "KEYS: ");
    for (u32 key = 0; key < p->slot_pool_len; ++key) {
        offset += snprintf(buf + offset, buf_sz - offset, " %2u  ", key);
    }
    offset += snprintf(buf + offset, buf_sz - offset, "\n");

    // Active row
    offset += snprintf(buf + offset, buf_sz - offset, "ACTV: ");
    for (u32 key = 0; key < p->slot_pool_len; ++key) {
        if (pool_alive(p, key)) {
            offset += snprintf(buf + offset, buf_sz - offset, "[%2u] ", static_cast<u32>(-p->slot_pool_ptr[key]));
        } else {
            offset += snprintf(buf + offset, buf_sz - offset, "[  ] ");
        }
    }
    offset += snprintf(buf + offset, buf_sz - offset, "\n");

    // Free row
    offset += snprintf(buf + offset, buf_sz - offset, "FREE: ");
    for (u32 key = 0; key < p->slot_pool_len; ++key) {
        if (!pool_alive(p, key)) {
            offset += snprintf(buf + offset, buf_sz - offset, "[%2u] ", static_cast<u32>(p->slot_pool_ptr[key]));
        } else {
            offset += snprintf(buf + offset, buf_sz - offset, "[  ] ");
        }
    }
    offset += snprintf(buf + offset, buf_sz - offset, "\n");

    log_info("%s", buf);
    free(buf);
}

}

