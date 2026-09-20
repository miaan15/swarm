package entity

import "../engine"
import "../engine/core"
import "../global"

COLLIDER_ALIVE_POOL_FLAG :: 0xFFFFFFFF

collider :: struct {
    key: u32,

    rect: [4]f32,
    tag: u32,

    // tree
    tree_node: u32,

    // chunk
    chunk_key: u32,

    // entity
    ett_owner: u32,
    ett_links: [2]u32,

    ett_size: [2]f32,
    ett_offset: [2]f32,
}

collider_node :: struct {
    pool_flag: u32,
    pool_idx: u32,

    collider_key: u32,

    parent: u32,
    child: [2]u32,
    height: i32,

    rect: [4]f32,
    tag: u32,
}

collider_sys : struct {
    collider_pool: pool(collider),
    collider_chunk: chunk_mng,

    // bvh
    tree_pool: [^]collider_node,
    tree_cap, tree_head, tree_max_idx, tree_len: u32,

    tree_root: u32,
    fat_aabb_offset: f32,
} = {}

// ================================================================================================
collider_sys_init :: proc(cap: u32, fat_aabb_offset: f32) {
    pool_init(&collider_sys.collider_pool, cap)
    chunk_mng_init(&collider_sys.collider_chunk, 1024, cap)

    collider_sys.tree_cap = 2 * cap
    collider_sys.tree_pool = cast([^]collider_node)core.arena_alloc(&global.omni_arena, collider_sys.tree_cap * size_of(collider_node))

    collider_sys.fat_aabb_offset = fat_aabb_offset

    collider_sys.tree_head = 1
    collider_sys.tree_max_idx = 1
    collider_sys.tree_len = 1
    collider_sys.tree_root = 0
}

// ================================================================================================
collider_create :: proc(rect: [4]f32 = {0, 0, 0, 0}, tag: u32 = 0) -> (_key: u32, _ptr: ^collider) {
    if collider_sys.collider_pool.len >= collider_sys.collider_pool.cap {
        core.log_error("collider_create: too many collider (%d) => stub", collider_sys.collider_pool.len)
        return 0, &collider_sys.collider_pool.data_list[0]
    }

    key, ptr := pool_create(&collider_sys.collider_pool)

    ptr.rect = rect
    ptr.tag = tag
    ptr.tree_node = 0

    ptr.chunk_key, _ = chunk_mng_create(&collider_sys.collider_chunk, key, chunk_cal_center_rect(ptr.rect))

    // FIXME
    collider_add_to_tree(key)

    core.log_debug("created collider [%d]: rect = (%.1f, %.1f, %.1f, %.1f); tag = %d", key, rect[0], rect[1], rect[2], rect[3], tag)

    return key, ptr
}

collider_destroy :: proc(key: u32) {
    if !pool_alive(&collider_sys.collider_pool, key) {
        core.log_error("collider_destroy: collider [%d] invalid (dead or worse)", key)
        return
    }

    ptr := pool_get(&collider_sys.collider_pool, key)

    chunk_mng_destroy(&collider_sys.collider_chunk, ptr.chunk_key)

    if ptr.tree_node != 0 { collider_remv_from_tree(key) }

    pool_destroy(&collider_sys.collider_pool, key)

    core.log_debug("destroyed collider [%d]", key)
}

collider_get :: proc(key: u32) -> ^collider {
    if !pool_alive(&collider_sys.collider_pool, key) {
        core.log_error("collider_destroy: collider [%d] invalid (dead or worse) => stub", key)
        return &collider_sys.collider_pool.data_list[0]
    }

    return pool_get(&collider_sys.collider_pool, key)
}

collider_alive :: proc(key: u32) -> bool {
    return pool_alive(&collider_sys.collider_pool, key)
}

// ================================================================================================
collider_add_to_tree :: proc(key: u32) {
    if !pool_alive(&collider_sys.collider_pool, key) {
        core.log_error("collider_add_to_tree: collider [%d] invalid (dead or worse)", key)
        return
    }

    col := pool_get(&collider_sys.collider_pool, key)
    if col.tree_node != 0 {
        core.log_warn("collider_add_to_tree: collider [%d] already in tree", key)
        return
    }

    col.tree_node = collider_tree_insert(key, true)
}

collider_remv_from_tree :: proc(key: u32) {
    if !pool_alive(&collider_sys.collider_pool, key) {
        core.log_error("collider_add_to_tree: collider [%d] invalid (dead or worse)", key)
        return
    }

    col := pool_get(&collider_sys.collider_pool, key)
    if col.tree_node == 0 {
        core.log_warn("collider_add_to_tree: collider [%d] already not in tree", key)
        return
    }

    collider_tree_remove(col.tree_node, true)
    col.tree_node = 0
}

// ================================================================================================
collider_sys_update :: proc() {
    _idx: u32 = 1
    for key, ptr in pool_iterate(&collider_sys.collider_pool, &_idx) {
        chunk_mng_update(&collider_sys.collider_chunk, key, chunk_cal_center_rect(ptr.rect))

        if ptr.tree_node != 0 {
            node := &collider_sys.tree_pool[ptr.tree_node]

            if !_collider_rect_contains(node.rect, ptr.rect) {
                collider_tree_remove(ptr.tree_node, false)
                collider_tree_insert(key, false)
            }
        }
    }
}

// ================================================================================================
collider_node_create :: proc(collider_key: u32) -> u32 {
    assert(collider_sys.tree_len < collider_sys.tree_cap)

    node_idx := collider_sys.tree_head
    col_node := &collider_sys.tree_pool[node_idx]

    if node_idx == collider_sys.tree_max_idx {
        collider_sys.tree_max_idx += 1
        collider_sys.tree_head += 1
    } else {
        collider_sys.tree_head = col_node.pool_flag
    }

    collider_sys.tree_len += 1

    col_node^ = collider_node{}
    col_node.pool_flag = COLLIDER_ALIVE_POOL_FLAG
    col_node.pool_idx = node_idx

    if collider_key != 0 {
        col := collider_get(collider_key)
        offset := collider_sys.fat_aabb_offset
        col_node.collider_key = collider_key
        col_node.rect = {
            col.rect[0] - offset,
            col.rect[1] - offset,
            col.rect[2] + 2 * offset,
            col.rect[3] + 2 * offset,
        }
        col_node.tag = col.tag
    }

    return node_idx
}

collider_node_destroy :: proc(idx: u32) {
    col_node := &collider_sys.tree_pool[idx]
    assert(col_node.pool_flag == COLLIDER_ALIVE_POOL_FLAG)

    col_node.pool_flag = collider_sys.tree_head
    collider_sys.tree_head = idx
    collider_sys.tree_len -= 1
}

collider_node_update :: proc(collider_key: u32) {
    assert(collider_key != 0 && collider_key < collider_sys.collider_pool.max_key)

    col := collider_get(collider_key)
    col_node := &collider_sys.tree_pool[col.tree_node]
    offset := collider_sys.fat_aabb_offset

    col_node.collider_key = collider_key
    col_node.rect = {
        col.rect[0] - offset,
        col.rect[1] - offset,
        col.rect[2] + 2 * offset,
        col.rect[3] + 2 * offset,
    }
    col_node.tag = col.tag
    col_node.child = {0, 0}
    col_node.parent = 0
    col_node.height = 0
}

// ================================================================================================
collider_tree_insert :: proc(collider_key: u32, create_new_node: bool) -> u32 {
    insert_idx: u32
    if create_new_node {
        insert_idx = collider_node_create(collider_key)
    } else {
        assert(collider_alive(collider_key))
        col := collider_get(collider_key)
        insert_idx = col.tree_node
        collider_node_update(collider_key)
    }

    if collider_sys.tree_root == 0 {
        collider_sys.tree_root = insert_idx
        return insert_idx
    }

    leaf_rect := collider_sys.tree_pool[insert_idx].rect

    // find sibling
    cur_idx := collider_sys.tree_root
    for !_collider_node_is_leaf(cur_idx) {
        cur_node := &collider_sys.tree_pool[cur_idx]
        child0_idx := cur_node.child[0]
        child1_idx := cur_node.child[1]
        child0 := &collider_sys.tree_pool[child0_idx]
        child1 := &collider_sys.tree_pool[child1_idx]

        area := _collider_rect_perimeter(cur_node.rect)
        combined := _collider_rect_union(cur_node.rect, leaf_rect)
        combined_area := _collider_rect_perimeter(combined)

        cost := 2.0 * combined_area
        inherit_cost := 2.0 * (combined_area - area)

        cost0: f32
        if _collider_node_is_leaf(child0_idx) {
            cost0 = _collider_rect_perimeter(_collider_rect_union(child0.rect, leaf_rect)) + inherit_cost
        } else {
            old_area := _collider_rect_perimeter(child0.rect)
            new_area := _collider_rect_perimeter(_collider_rect_union(child0.rect, leaf_rect))
            cost0 = (new_area - old_area) + inherit_cost
        }

        cost1: f32
        if _collider_node_is_leaf(child1_idx) {
            cost1 = _collider_rect_perimeter(_collider_rect_union(child1.rect, leaf_rect)) + inherit_cost
        } else {
            old_area := _collider_rect_perimeter(child1.rect)
            new_area := _collider_rect_perimeter(_collider_rect_union(child1.rect, leaf_rect))
            cost1 = (new_area - old_area) + inherit_cost
        }

        if cost < cost0 && cost < cost1 {
            break
        }

        cur_idx = (cost0 < cost1) ? child0_idx : child1_idx
    }

    sibling_idx := cur_idx

    // common parent
    old_parent_idx := collider_sys.tree_pool[sibling_idx].parent
    new_parent_idx := collider_node_create(0)

    // linking
    new_parent := &collider_sys.tree_pool[new_parent_idx]
    new_parent.parent = old_parent_idx
    new_parent.rect = _collider_rect_union(leaf_rect, collider_sys.tree_pool[sibling_idx].rect)
    new_parent.height = collider_sys.tree_pool[sibling_idx].height + 1
    new_parent.child[0] = sibling_idx
    new_parent.child[1] = insert_idx

    collider_sys.tree_pool[sibling_idx].parent = new_parent_idx
    collider_sys.tree_pool[insert_idx].parent = new_parent_idx

    if old_parent_idx != 0 {
        old_parent := &collider_sys.tree_pool[old_parent_idx]
        if old_parent.child[0] == sibling_idx {
            old_parent.child[0] = new_parent_idx
        } else {
            old_parent.child[1] = new_parent_idx
        }
    } else {
        collider_sys.tree_root = new_parent_idx
    }

    // walkup + rebalance
    walk_idx := new_parent_idx
    for walk_idx != 0 {
        walk_idx = _collider_tree_balance(walk_idx)

        p := &collider_sys.tree_pool[walk_idx]
        c0 := &collider_sys.tree_pool[p.child[0]]
        c1 := &collider_sys.tree_pool[p.child[1]]

        p.height = 1 + max(c0.height, c1.height)
        p.rect = _collider_rect_union(c0.rect, c1.rect)
        p.tag = c0.tag | c1.tag

        walk_idx = p.parent
    }

    return insert_idx
}

collider_tree_remove :: proc(node_idx: u32, destroy_node: bool) {
    assert(_collider_node_is_leaf(node_idx))

    if node_idx == collider_sys.tree_root {
        if destroy_node {
            collider_node_destroy(node_idx)
        }
        collider_sys.tree_root = 0
        return
    }

    remv_node := &collider_sys.tree_pool[node_idx]
    par_idx := remv_node.parent
    par_node := &collider_sys.tree_pool[par_idx]
    grandpar_idx := par_node.parent

    sibling_idx := (par_node.child[0] == node_idx) ? par_node.child[1] : par_node.child[0]
    sibling_node := &collider_sys.tree_pool[sibling_idx]

    if grandpar_idx != 0 {
        grandpar_node := &collider_sys.tree_pool[grandpar_idx]
        if grandpar_node.child[0] == par_idx {
            grandpar_node.child[0] = sibling_idx
        } else {
            grandpar_node.child[1] = sibling_idx
        }
        sibling_node.parent = grandpar_idx

        if destroy_node {
            collider_node_destroy(node_idx)
        }
        collider_node_destroy(par_idx)

        // walkup
        walk_idx := grandpar_idx
        for walk_idx != 0 {
            walk_idx = _collider_tree_balance(walk_idx)

            p := &collider_sys.tree_pool[walk_idx]
            c0 := &collider_sys.tree_pool[p.child[0]]
            c1 := &collider_sys.tree_pool[p.child[1]]

            p.rect = _collider_rect_union(c0.rect, c1.rect)
            p.height = 1 + max(c0.height, c1.height)
            p.tag = c0.tag | c1.tag

            walk_idx = p.parent
        }
    } else {
        collider_sys.tree_root = sibling_idx
        sibling_node.parent = 0

        if destroy_node {
            collider_node_destroy(node_idx)
        }
        collider_node_destroy(par_idx)
    }
}

// PRIVATE
// ================================================================================================
_collider_rect_perimeter :: proc(r: [4]f32) -> f32 {
    return 2.0 * (r[2] + r[3])
}

_collider_rect_union :: proc(a, b: [4]f32) -> [4]f32 {
    min_x := min(a[0], b[0])
    min_y := min(a[1], b[1])
    max_x := max(a[0] + a[2], b[0] + b[2])
    max_y := max(a[1] + a[3], b[1] + b[3])
    return {min_x, min_y, max_x - min_x, max_y - min_y}
}

_collider_rect_contains :: proc(fat, r: [4]f32) -> bool {
    return r[0] >= fat[0] &&
           r[1] >= fat[1] &&
           r[0] + r[2] <= fat[0] + fat[2] &&
           r[1] + r[3] <= fat[1] + fat[3]
}

_collider_node_is_leaf :: proc(idx: u32) -> bool {
    return collider_sys.tree_pool[idx].child[0] == 0
}

_collider_tree_balance :: proc(iA: u32) -> u32 {
    A := &collider_sys.tree_pool[iA]
    if _collider_node_is_leaf(iA) || A.height < 2 {
        return iA
    }

    iB := A.child[0]
    iC := A.child[1]
    B := &collider_sys.tree_pool[iB] // -1
    C := &collider_sys.tree_pool[iC] // 1

    balance := C.height - B.height

    // C
    if balance > 1 {
        iF := C.child[0]
        iG := C.child[1]
        F := &collider_sys.tree_pool[iF]
        G := &collider_sys.tree_pool[iG]

        C.child[0] = iA
        C.parent = A.parent
        A.parent = iC

        if C.parent != 0 {
            p := &collider_sys.tree_pool[C.parent]
            if p.child[0] == iA {
                p.child[0] = iC
            } else {
                p.child[1] = iC
            }
        } else {
            collider_sys.tree_root = iC
        }

        if F.height > G.height {
            C.child[1] = iF
            A.child[1] = iG
            G.parent = iA

            A.rect = _collider_rect_union(B.rect, G.rect)
            C.rect = _collider_rect_union(A.rect, F.rect)

            A.height = 1 + max(B.height, G.height)
            C.height = 1 + max(A.height, F.height)
        } else {
            C.child[1] = iG
            A.child[1] = iF
            F.parent = iA

            A.rect = _collider_rect_union(B.rect, F.rect)
            C.rect = _collider_rect_union(A.rect, G.rect)

            A.height = 1 + max(B.height, F.height)
            C.height = 1 + max(A.height, G.height)
        }
        return iC
    }

    // B
    if balance < -1 {
        iD := B.child[0]
        iE := B.child[1]
        D := &collider_sys.tree_pool[iD]
        E := &collider_sys.tree_pool[iE]

        B.child[0] = iA
        B.parent = A.parent
        A.parent = iB

        if B.parent != 0 {
            p := &collider_sys.tree_pool[B.parent]
            if p.child[0] == iA {
                p.child[0] = iB
            } else {
                p.child[1] = iB
            }
        } else {
            collider_sys.tree_root = iB
        }

        if D.height > E.height {
            B.child[1] = iD
            A.child[0] = iE
            E.parent = iA

            A.rect = _collider_rect_union(C.rect, E.rect)
            B.rect = _collider_rect_union(A.rect, D.rect)

            A.height = 1 + max(C.height, E.height)
            B.height = 1 + max(A.height, D.height)
        } else {
            B.child[1] = iE
            A.child[0] = iD
            D.parent = iA

            A.rect = _collider_rect_union(C.rect, D.rect)
            B.rect = _collider_rect_union(A.rect, E.rect)

            A.height = 1 + max(C.height, D.height)
            B.height = 1 + max(A.height, E.height)
        }
        return iB
    }

    return iA
}

// TEST
// ================================================================================================
_collider_debug_draw :: proc() {
    if collider_sys.tree_root != 0 {
        _collider_debug_draw_node_recursive(&collider_sys.tree_pool[collider_sys.tree_root])
    }

    _idx: u32 = 1
    for key, ptr in pool_iterate(&collider_sys.collider_pool, &_idx) {
        draw := engine.draw_make()
        draw.type = .RECTANGLE
        draw.rectangle.rect = ptr.rect
        draw.rectangle.color = {0, 255, 0, 255}
    }
}

_collider_debug_draw_node_recursive :: proc(node: ^collider_node) {
    draw := engine.draw_make()
    draw.type = .RECTANGLE
    draw.rectangle.rect = node.rect
    draw.rectangle.color = {255, 255, 255, 255}

    if node.child[0] != 0 { _collider_debug_draw_node_recursive(&collider_sys.tree_pool[node.child[0]]) }
    if node.child[1] != 0 { _collider_debug_draw_node_recursive(&collider_sys.tree_pool[node.child[1]]) }
}

