// collider, detect hitting, should be straight forward
// - this suppose to be entity's component, but still can be stand alone

module;

#include <cassert>
#include <cstddef>

export module entity:collider;

import def;
import mem;
import log;

import pool;
import pool_simple;

import draw;

import context;

import :chunk;

export namespace sw {

constexpr f32 COLLIDER_MAX_BOUNDS_SIZE = 256;
constexpr f32 COLLIDER_CHUNK_SIZE = 1024;

struct collider {
    u32 pool_key;

    f32 rect[4];
    u32 tag;

    // tree
    u32 tree_node_idx;

    // chunk
    u32 chunk_key;

    // entity
    u32 owner_entity_key;
    u32 links_in_entity_list[2];

    // entity properties
    f32 size_property_entity[2];
    f32 offset_property_entity[2];

    // once there's entity owner, the entity properties will control the collider data (rect)
};

struct collider_tree_node {
    // all tree node actually store in a pool
    u32 pool_key;

    // collider
    u32 collider_key;
    f32 collider_rect[4];
    u32 collider_tag;

    // tree links
    u32 parent;
    u32 child[2];
    u32 height;
};

struct {
    // collider
    pool<collider> collider_pool;
    chunk_mng collider_chunk;

    // tree node pool
    pool_simple<collider_tree_node> tree_node_pool;

    // tree
    u32 tree_root_node_idx;

    f32 fat_aabb_offset;
} collider_sys = {};

// ================================================================================================

// collider rect, node utilities
f32 _collider_rect_perimeter(f32 r[4]);
void _collider_rect_union(f32 a[4], f32 b[4], f32 out_r[4]);
bool _collider_rect_contains(f32 fat[4], f32 r[4]);
bool _collider_rect_overlaps(f32 a[4], f32 b[4]);
bool _collider_node_is_leaf(u32 idx);

// collider and tree stuff
void collider_add_to_tree(u32 collider_key);
void collider_remv_from_tree(u32 collider_key);
// helper for aboves
u32 _collider_tree_insert(u32 collider_key, bool is_create_new_node);
void _collider_tree_remove(u32 node_idx, bool is_destroy_node);
u32 _collider_tree_balance(u32 iA);
void _collider_tree_process_pairs(u32 idxA, u32 idxB, void (*fn_process)(collider*, collider*));
void _collider_debug_draw_node_recursive(collider_tree_node *node);

// ================================================================================================

void collider_sys_init(u32 cap, f32 fat_aabb_offset) {
    pool_init(&collider_sys.collider_pool, cap, offsetof(collider, pool_key));
    chunk_mng_init(&collider_sys.collider_chunk, cap, COLLIDER_CHUNK_SIZE, COLLIDER_MAX_BOUNDS_SIZE);

    pool_simple_init(&collider_sys.tree_node_pool, 2 * cap, offsetof(collider_tree_node, pool_key));

    collider_sys.fat_aabb_offset = fat_aabb_offset;
    collider_sys.tree_root_node_idx = 0;
}

// ================================================================================================

void collider_create(f32 rect[4], u32 tag, u32 *out_collider_key, collider **out_collider_ptr) {
    // create collider, update chunk, add to BVH

    if (collider_sys.collider_pool.data_list_len >= collider_sys.collider_pool.cap) {
        log_err("collider_create: too many collider (%u) => stub", collider_sys.collider_pool.data_list_len);
        if (out_collider_key) { *out_collider_key = 0; }
        if (out_collider_ptr) { *out_collider_ptr = &collider_sys.collider_pool.data_list_ptr[0]; }
        return;
    }

    u32 collider_key = 0;
    collider *collider_ptr = nullptr;
    pool_create(&collider_sys.collider_pool, &collider_key, &collider_ptr);

    if (rect) {
        collider_ptr->rect[0] = rect[0];
        collider_ptr->rect[1] = rect[1];
        collider_ptr->rect[2] = rect[2];
        collider_ptr->rect[3] = rect[3];
    } else {
        collider_ptr->rect[0] = 0;
        collider_ptr->rect[1] = 0;
        collider_ptr->rect[2] = 0;
        collider_ptr->rect[3] = 0;
    }

    collider_ptr->tag = tag;
    collider_ptr->tree_node_idx = 0;

    // chunk
    f32 center_pos[2];
    chunk_cal_center_rect(collider_ptr->rect, center_pos);
    chunk_mng_create(&collider_sys.collider_chunk, collider_key, center_pos, &collider_ptr->chunk_key, nullptr);

    // add to BVH
    collider_add_to_tree(collider_key);

    log_debug("created collider [%u]: rect = (%.1f, %.1f, %.1f, %.1f); tag = %u", collider_key, collider_ptr->rect[0], collider_ptr->rect[1], collider_ptr->rect[2], collider_ptr->rect[3], tag);

    if (out_collider_key) { *out_collider_key = collider_key; }
    if (out_collider_ptr) { *out_collider_ptr = collider_ptr; }
}

void collider_destroy(u32 collider_key) {
    if (!pool_alive(&collider_sys.collider_pool, collider_key)) {
        log_err("collider_destroy: collider [%u] invalid (dead or worse)", collider_key);
        return;
    }

    collider *collider_ptr = pool_get(&collider_sys.collider_pool, collider_key);

    // destroy from systems (chunk, tree)
    chunk_mng_destroy(&collider_sys.collider_chunk, collider_ptr->chunk_key);

    if (collider_ptr->tree_node_idx != 0) {
        collider_remv_from_tree(collider_key);
    }

    pool_destroy(&collider_sys.collider_pool, collider_key);

    log_debug("destroyed collider [%u]", collider_key);
}

collider *collider_get(u32 collider_key) {
    if (!pool_alive(&collider_sys.collider_pool, collider_key)) {
        log_err("collider_destroy: collider [%u] invalid (dead or worse) => stub", collider_key);
        return &collider_sys.collider_pool.data_list_ptr[0];
    }

    return pool_get(&collider_sys.collider_pool, collider_key);
}

bool collider_alive(u32 collider_key) {
    return pool_alive(&collider_sys.collider_pool, collider_key);
}

// ================================================================================================

void collider_sys_update() {
    u32 iter_idx = 0;
    u32 collider_key = 0;
    collider *collider_ptr = nullptr;

    while (pool_iterate(&collider_sys.collider_pool, &iter_idx, &collider_key, &collider_ptr)) {
        // update chunk
        f32 center_pos[2];
        chunk_cal_center_rect(collider_ptr->rect, center_pos);
        chunk_mng_update(&collider_sys.collider_chunk, collider_key, center_pos);

        // re-insert tree if moved out of fat aabb bounds
        if (collider_ptr->tree_node_idx != 0) {
            collider_tree_node *node = pool_simple_get(&collider_sys.tree_node_pool, collider_ptr->tree_node_idx);

            if (!_collider_rect_contains(node->collider_rect, collider_ptr->rect)) {
                _collider_tree_remove(collider_ptr->tree_node_idx, false);
                _collider_tree_insert(collider_key, false);
            }
        }
    }

    // broadphase query pairs
    if (collider_sys.tree_root_node_idx != 0) {
        _collider_tree_process_pairs(collider_sys.tree_root_node_idx, collider_sys.tree_root_node_idx, nullptr);
    }
}

// ================================================================================================

void collider_add_to_tree(u32 collider_key) {
    if (!pool_alive(&collider_sys.collider_pool, collider_key)) {
        log_err("collider_add_to_tree: collider [%u] invalid (dead or worse)", collider_key);
        return;
    }

    collider *col = pool_get(&collider_sys.collider_pool, collider_key);
    if (col->tree_node_idx != 0) {
        log_warn("collider_add_to_tree: collider [%u] already in tree", collider_key);
        return;
    }

    col->tree_node_idx = _collider_tree_insert(collider_key, true);
}

void collider_remv_from_tree(u32 collider_key) {
    if (!pool_alive(&collider_sys.collider_pool, collider_key)) {
        log_err("collider_add_to_tree: collider [%u] invalid (dead or worse)", collider_key);
        return;
    }

    collider *col = pool_get(&collider_sys.collider_pool, collider_key);
    if (col->tree_node_idx == 0) {
        log_warn("collider_add_to_tree: collider [%u] already not in tree", collider_key);
        return;
    }

    _collider_tree_remove(col->tree_node_idx, true);
    col->tree_node_idx = 0;
}

// ================================================================================================

u32 _collider_tree_insert(u32 collider_key, bool is_create_new_node) {
    // insert a leaf node into BVH tree
    // - create a node (or re-cycle old one)
    // - find best sibling using SAH cost
    // - create a new internal node to become common parent of sibling & leaf
    // - walk up, update, rebalance

    u32 insert_idx = 0;
    collider_tree_node *insert_node = nullptr;

    if (is_create_new_node) {
        pool_simple_create(&collider_sys.tree_node_pool, &insert_idx, &insert_node);
    } else {
        assert(collider_alive(collider_key));
        collider *col = collider_get(collider_key);
        insert_idx = col->tree_node_idx;
        insert_node = pool_simple_get(&collider_sys.tree_node_pool, insert_idx);
        insert_node->child[0] = 0;
        insert_node->child[1] = 0;
        insert_node->parent = 0;
        insert_node->height = 0;
    }

    collider *col = collider_get(collider_key);
    f32 offset = collider_sys.fat_aabb_offset;
    insert_node->collider_key = collider_key;
    insert_node->collider_rect[0] = col->rect[0] - offset;
    insert_node->collider_rect[1] = col->rect[1] - offset;
    insert_node->collider_rect[2] = col->rect[2] + 2 * offset;
    insert_node->collider_rect[3] = col->rect[3] + 2 * offset;
    insert_node->collider_tag = col->tag;

    // empty tree => inserted leaf becomes root
    if (collider_sys.tree_root_node_idx == 0) {
        collider_sys.tree_root_node_idx = insert_idx;
        return insert_idx;
    }

    f32 leaf_rect[4] = {
        insert_node->collider_rect[0],
        insert_node->collider_rect[1],
        insert_node->collider_rect[2],
        insert_node->collider_rect[3]
    };

    // find best sibling down the tree use SAH
    // - cost: penalty if we pair directly with cur_node (creates new parent here)
    // - cost0/cost1: penalty if we descend into child0 or child1 instead
    // - inherit_cost: cost from all cur_node's desendants
    u32 cur_idx = collider_sys.tree_root_node_idx;
    while (!_collider_node_is_leaf(cur_idx)) {
        collider_tree_node *cur_node = pool_simple_get(&collider_sys.tree_node_pool, cur_idx);
        u32 child0_idx = cur_node->child[0];
        u32 child1_idx = cur_node->child[1];
        collider_tree_node *child0 = pool_simple_get(&collider_sys.tree_node_pool, child0_idx);
        collider_tree_node *child1 = pool_simple_get(&collider_sys.tree_node_pool, child1_idx);

        f32 area = _collider_rect_perimeter(cur_node->collider_rect);
        f32 combined[4];
        _collider_rect_union(cur_node->collider_rect, leaf_rect, combined);
        f32 combined_area = _collider_rect_perimeter(combined);

        f32 cost = 2 * combined_area;
        f32 inherit_cost = 2 * (combined_area - area);

        // cost if to child[0]
        f32 cost0 = 0;
        if (_collider_node_is_leaf(child0_idx)) {
            f32 comb0[4];
            _collider_rect_union(child0->collider_rect, leaf_rect, comb0);
            cost0 = _collider_rect_perimeter(comb0) + inherit_cost;
        } else {
            f32 old_area = _collider_rect_perimeter(child0->collider_rect);
            f32 comb0[4];
            _collider_rect_union(child0->collider_rect, leaf_rect, comb0);
            f32 new_area = _collider_rect_perimeter(comb0);
            cost0 = (new_area - old_area) + inherit_cost;
        }

        // cost if to child[1]
        f32 cost1 = 0;
        if (_collider_node_is_leaf(child1_idx)) {
            f32 comb1[4];
            _collider_rect_union(child1->collider_rect, leaf_rect, comb1);
            cost1 = _collider_rect_perimeter(comb1) + inherit_cost;
        } else {
            f32 old_area = _collider_rect_perimeter(child1->collider_rect);
            f32 comb1[4];
            _collider_rect_union(child1->collider_rect, leaf_rect, comb1);
            f32 new_area = _collider_rect_perimeter(comb1);
            cost1 = (new_area - old_area) + inherit_cost;
        }

        // stop her is cheaper than descending
        if (cost < cost0 && cost < cost1) {
            break;
        }

        // to cheaper branch
        cur_idx = (cost0 < cost1) ? child0_idx : child1_idx;
    }

    u32 sibling_idx = cur_idx;
    collider_tree_node *sibling_node = pool_simple_get(&collider_sys.tree_node_pool, sibling_idx);

    // common parent of sibling & inserted leaf
    u32 old_parent_idx = sibling_node->parent;
    u32 new_parent_idx = 0;
    collider_tree_node *new_parent = nullptr;
    pool_simple_create(&collider_sys.tree_node_pool, &new_parent_idx, &new_parent);

    // new_parent will be between old_parent and 2 childs
    new_parent->parent = old_parent_idx;
    _collider_rect_union(leaf_rect, sibling_node->collider_rect, new_parent->collider_rect);
    new_parent->height = sibling_node->height + 1;
    new_parent->child[0] = sibling_idx;
    new_parent->child[1] = insert_idx;

    sibling_node->parent = new_parent_idx;
    pool_simple_get(&collider_sys.tree_node_pool, insert_idx)->parent = new_parent_idx;

    // link new parent
    if (old_parent_idx != 0) {
        collider_tree_node *old_parent = pool_simple_get(&collider_sys.tree_node_pool, old_parent_idx);
        if (old_parent->child[0] == sibling_idx) {
            old_parent->child[0] = new_parent_idx;
        } else {
            old_parent->child[1] = new_parent_idx;
        }
    } else {
        collider_sys.tree_root_node_idx = new_parent_idx;
    }

    // walkup + rebalance
    u32 walk_idx = new_parent_idx;
    while (walk_idx != 0) {
        walk_idx = _collider_tree_balance(walk_idx);

        collider_tree_node *p = pool_simple_get(&collider_sys.tree_node_pool, walk_idx);
        collider_tree_node *c0 = pool_simple_get(&collider_sys.tree_node_pool, p->child[0]);
        collider_tree_node *c1 = pool_simple_get(&collider_sys.tree_node_pool, p->child[1]);

        p->height = 1 + ((c0->height > c1->height) ? c0->height : c1->height);
        _collider_rect_union(c0->collider_rect, c1->collider_rect, p->collider_rect);
        p->collider_tag = c0->collider_tag | c1->collider_tag;

        walk_idx = p->parent;
    }

    return insert_idx;
}

void _collider_tree_remove(u32 node_idx, bool is_destroy_node) {
    // remove a leaf node from BVH tree
    // - disconnect leaf and its parent
    // - promote sibling into grandparent's child slot
    // - destroy parent internal node (and desttroy leaf node or not)
    // - walk up, update, rebalance

    assert(_collider_node_is_leaf(node_idx));

    // removing root leaf => tree empty
    if (node_idx == collider_sys.tree_root_node_idx) {
        if (is_destroy_node) {
            pool_simple_destroy(&collider_sys.tree_node_pool, node_idx);
        }
        collider_sys.tree_root_node_idx = 0;
        return;
    }

    collider_tree_node *remv_node = pool_simple_get(&collider_sys.tree_node_pool, node_idx);
    u32 par_idx = remv_node->parent;
    collider_tree_node *par_node = pool_simple_get(&collider_sys.tree_node_pool, par_idx);
    u32 grandpar_idx = par_node->parent;

    u32 sibling_idx = (par_node->child[0] == node_idx) ? par_node->child[1] : par_node->child[0];
    collider_tree_node *sibling_node = pool_simple_get(&collider_sys.tree_node_pool, sibling_idx);

    if (grandpar_idx != 0) {
        // connect sibling to grandparent
        collider_tree_node *grandpar_node = pool_simple_get(&collider_sys.tree_node_pool, grandpar_idx);
        if (grandpar_node->child[0] == par_idx) {
            grandpar_node->child[0] = sibling_idx;
        } else {
            grandpar_node->child[1] = sibling_idx;
        }
        sibling_node->parent = grandpar_idx;

        if (is_destroy_node) {
            pool_simple_destroy(&collider_sys.tree_node_pool, node_idx);
        }
        pool_simple_destroy(&collider_sys.tree_node_pool, par_idx);

        // walkup from grandparent
        u32 walk_idx = grandpar_idx;
        while (walk_idx != 0) {
            walk_idx = _collider_tree_balance(walk_idx);

            collider_tree_node *p = pool_simple_get(&collider_sys.tree_node_pool, walk_idx);
            collider_tree_node *c0 = pool_simple_get(&collider_sys.tree_node_pool, p->child[0]);
            collider_tree_node *c1 = pool_simple_get(&collider_sys.tree_node_pool, p->child[1]);

            _collider_rect_union(c0->collider_rect, c1->collider_rect, p->collider_rect);
            p->height = 1 + ((c0->height > c1->height) ? c0->height : c1->height);
            p->collider_tag = c0->collider_tag | c1->collider_tag;

            walk_idx = p->parent;
        }
    } else {
        // parent was root => sibling = root
        collider_sys.tree_root_node_idx = sibling_idx;
        sibling_node->parent = 0;

        if (is_destroy_node) {
            pool_simple_destroy(&collider_sys.tree_node_pool, node_idx);
        }
        pool_simple_destroy(&collider_sys.tree_node_pool, par_idx);
    }
}

// ================================================================================================

f32 _collider_rect_perimeter(f32 r[4]) {
    return 2 * (r[2] + r[3]);
}

void _collider_rect_union(f32 a[4], f32 b[4], f32 out_r[4]) {
    f32 min_x = (a[0] < b[0]) ? a[0] : b[0];
    f32 min_y = (a[1] < b[1]) ? a[1] : b[1];
    f32 max_x = (a[0] + a[2] > b[0] + b[2]) ? (a[0] + a[2]) : (b[0] + b[2]);
    f32 max_y = (a[1] + a[3] > b[1] + b[3]) ? (a[1] + a[3]) : (b[1] + b[3]);

    out_r[0] = min_x;
    out_r[1] = min_y;
    out_r[2] = max_x - min_x;
    out_r[3] = max_y - min_y;
}

bool _collider_rect_contains(f32 fat[4], f32 r[4]) {
    return r[0] >= fat[0] &&
           r[1] >= fat[1] &&
           r[0] + r[2] <= fat[0] + fat[2] &&
           r[1] + r[3] <= fat[1] + fat[3];
}

bool _collider_rect_overlaps(f32 a[4], f32 b[4]) {
    return a[0] < b[0] + b[2] &&
           a[0] + a[2] > b[0] &&
           a[1] < b[1] + b[3] &&
           a[1] + a[3] > b[1];
}

bool _collider_node_is_leaf(u32 idx) {
    return pool_simple_get(&collider_sys.tree_node_pool, idx)->child[0] == 0;
}

void _collider_tree_process_pairs(u32 idxA, u32 idxB, void (*fn_process)(collider*, collider*)) {
    // broadphase detect all collision pair
    // - prune search early if node aabb bounding boxes do not overlap
    // - both leaves => narrowphase: aabb check on actual colliders
    // - internal nodes => split node to check and descend

    if (idxA == 0 || idxB == 0) return;

    collider_tree_node *nodeA = pool_simple_get(&collider_sys.tree_node_pool, idxA);
    collider_tree_node *nodeB = pool_simple_get(&collider_sys.tree_node_pool, idxB);

    // early prune
    if (!_collider_rect_overlaps(nodeA->collider_rect, nodeB->collider_rect)) return;

    bool is_leafA = _collider_node_is_leaf(idxA);
    bool is_leafB = _collider_node_is_leaf(idxB);

    // both leaves => check overlap
    if (is_leafA && is_leafB) {
        if (idxA != idxB) {
            collider *colA = collider_get(nodeA->collider_key);
            collider *colB = collider_get(nodeB->collider_key);

            if (_collider_rect_overlaps(colA->rect, colB->rect)) {
                if (fn_process != nullptr) {
                    fn_process(colA, colB);
                }
            }
        }
        return;
    }

    // self collision pass: split node against its own children and cross its children
    if (idxA == idxB) {
        _collider_tree_process_pairs(nodeA->child[0], nodeA->child[0], fn_process);
        _collider_tree_process_pairs(nodeA->child[1], nodeA->child[1], fn_process);
        _collider_tree_process_pairs(nodeA->child[0], nodeA->child[1], fn_process);
        return;
    }

    // cross query pass: keep leaf intact while descending into non-leaf,
    // or split the higher subtree to keep tree balance
    if (is_leafA) {
        _collider_tree_process_pairs(idxA, nodeB->child[0], fn_process);
        _collider_tree_process_pairs(idxA, nodeB->child[1], fn_process);
    } else if (is_leafB) {
        _collider_tree_process_pairs(nodeA->child[0], idxB, fn_process);
        _collider_tree_process_pairs(nodeA->child[1], idxB, fn_process);
    } else if (nodeA->height > nodeB->height) {
        _collider_tree_process_pairs(nodeA->child[0], idxB, fn_process);
        _collider_tree_process_pairs(nodeA->child[1], idxB, fn_process);
    } else {
        _collider_tree_process_pairs(idxA, nodeB->child[0], fn_process);
        _collider_tree_process_pairs(idxA, nodeB->child[1], fn_process);
    }
}

u32 _collider_tree_balance(u32 iA) {
    // AVL-like tree rotations
    // node A has children B and C (B => left, C => right)
    // balance = height(C) - height(B):
    //    > 1  => right heavy, rotate C up
    //    < -1 => left heavy, rotate B up
    collider_tree_node *A = pool_simple_get(&collider_sys.tree_node_pool, iA);
    if (_collider_node_is_leaf(iA) || A->height < 2) {
        return iA;
    }

    u32 iB = A->child[0];
    u32 iC = A->child[1];
    collider_tree_node *B = pool_simple_get(&collider_sys.tree_node_pool, iB);
    collider_tree_node *C = pool_simple_get(&collider_sys.tree_node_pool, iC);

    i32 balance = C->height - B->height;

    // right heavy: rotate C up to replace A
    //    A                   C
    //   / \                 / \
    //  B   C      ==>      A   G (or F)
    //     / \             / \
    //    F   G           B   F (or G)
    if (balance > 1) {
        u32 iF = C->child[0];
        u32 iG = C->child[1];
        collider_tree_node *F = pool_simple_get(&collider_sys.tree_node_pool, iF);
        collider_tree_node *G = pool_simple_get(&collider_sys.tree_node_pool, iG);

        // swap A, C
        C->child[0] = iA;
        C->parent = A->parent;
        A->parent = iC;

        // update grandparent
        if (C->parent != 0) {
            collider_tree_node *p = pool_simple_get(&collider_sys.tree_node_pool, C->parent);
            if (p->child[0] == iA) {
                p->child[0] = iC;
            } else {
                p->child[1] = iC;
            }
        } else {
            collider_sys.tree_root_node_idx = iC;
        }

        // higher grandchild will be rotate to other branch for better balance
        if (F->height > G->height) {
            C->child[1] = iF;
            A->child[1] = iG;
            G->parent = iA;

            _collider_rect_union(B->collider_rect, G->collider_rect, A->collider_rect);
            _collider_rect_union(A->collider_rect, F->collider_rect, C->collider_rect);

            A->height = 1 + ((B->height > G->height) ? B->height : G->height);
            C->height = 1 + ((A->height > F->height) ? A->height : F->height);
        } else {
            C->child[1] = iG;
            A->child[1] = iF;
            F->parent = iA;

            _collider_rect_union(B->collider_rect, F->collider_rect, A->collider_rect);
            _collider_rect_union(A->collider_rect, G->collider_rect, C->collider_rect);

            A->height = 1 + ((B->height > F->height) ? B->height : F->height);
            C->height = 1 + ((A->height > G->height) ? A->height : G->height);
        }
        return iC;
    }

    // left heavy: rotate B up to replace A
    //      A                B
    //     / \              / \
    //    B   C    ==>     D   A (or E)
    //   / \                  / \
    //  D   E                E   C (or D)
    if (balance < -1) {
        u32 iD = B->child[0];
        u32 iE = B->child[1];
        collider_tree_node *D = pool_simple_get(&collider_sys.tree_node_pool, iD);
        collider_tree_node *E = pool_simple_get(&collider_sys.tree_node_pool, iE);

        // swap A, B
        B->child[0] = iA;
        B->parent = A->parent;
        A->parent = iB;

        // update grandparent
        if (B->parent != 0) {
            collider_tree_node *p = pool_simple_get(&collider_sys.tree_node_pool, B->parent);
            if (p->child[0] == iA) {
                p->child[0] = iB;
            } else {
                p->child[1] = iB;
            }
        } else {
            collider_sys.tree_root_node_idx = iB;
        }

        // higher grandchild will be rotate to other branch for better balance
        if (D->height > E->height) {
            B->child[1] = iD;
            A->child[0] = iE;
            E->parent = iA;

            _collider_rect_union(C->collider_rect, E->collider_rect, A->collider_rect);
            _collider_rect_union(A->collider_rect, D->collider_rect, B->collider_rect);

            A->height = 1 + ((C->height > E->height) ? C->height : E->height);
            B->height = 1 + ((A->height > D->height) ? A->height : D->height);
        } else {
            B->child[1] = iE;
            A->child[0] = iD;
            D->parent = iA;

            _collider_rect_union(C->collider_rect, D->collider_rect, A->collider_rect);
            _collider_rect_union(A->collider_rect, E->collider_rect, B->collider_rect);

            A->height = 1 + ((C->height > D->height) ? C->height : D->height);
            B->height = 1 + ((A->height > E->height) ? A->height : E->height);
        }
        return iB;
    }

    return iA;
}

// ================================================================================================

void _collider_debug_draw() {
    if (collider_sys.tree_root_node_idx != 0) {
        _collider_debug_draw_node_recursive(pool_simple_get(&collider_sys.tree_node_pool, collider_sys.tree_root_node_idx));
    }

    u32 iter_idx = 0;
    u32 collider_key = 0;
    collider *collider_ptr = nullptr;

    while (pool_iterate(&collider_sys.collider_pool, &iter_idx, &collider_key, &collider_ptr)) {
        draw_call *drw = draw_call_make();
        if (drw) {
            drw->type = draw_type::RECTANGLE;
            drw->rectangle.rect[0] = collider_ptr->rect[0];
            drw->rectangle.rect[1] = collider_ptr->rect[1];
            drw->rectangle.rect[2] = collider_ptr->rect[2];
            drw->rectangle.rect[3] = collider_ptr->rect[3];
            drw->rectangle.thickness = 1;
            drw->rectangle.color[0] = 0;
            drw->rectangle.color[1] = 255;
            drw->rectangle.color[2] = 0;
            drw->rectangle.color[3] = 255;
        }
    }
}

void _collider_debug_draw_node_recursive(collider_tree_node *node) {
    draw_call *drw = draw_call_make();
    if (drw) {
        drw->type = draw_type::RECTANGLE;
        drw->rectangle.rect[0] = node->collider_rect[0];
        drw->rectangle.rect[1] = node->collider_rect[1];
        drw->rectangle.rect[2] = node->collider_rect[2];
        drw->rectangle.rect[3] = node->collider_rect[3];
        drw->rectangle.thickness = 1;
        drw->rectangle.color[0] = 255;
        drw->rectangle.color[1] = 255;
        drw->rectangle.color[2] = 255;
        drw->rectangle.color[3] = 255;
    }

    if (node->child[0] != 0) {
        _collider_debug_draw_node_recursive(pool_simple_get(&collider_sys.tree_node_pool, node->child[0]));
    }
    if (node->child[1] != 0) {
        _collider_debug_draw_node_recursive(pool_simple_get(&collider_sys.tree_node_pool, node->child[1]));
    }
}

}
