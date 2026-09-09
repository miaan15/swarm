#include "box.h"

#include "context.h"
#include "log.h"
#include <assert.h>
#include <math.h>
#include <raylib.h>

struct box_sys box_sys = {0};

// =============================================================================
void box_tree_insert(u32 box_idx);

// =============================================================================
void box_sys_init(usize cap) {
    // box pool
    box_sys.box_pool = arena_alloc(&omni_arena, cap * sizeof(box));
    box_sys.box_cap = cap;

    // tree
    box_sys.tree_pool = arena_alloc(&omni_arena, (2 * cap) * sizeof(box_node));
    box_sys.tree_cap = 2 * cap;

    // stub
    box_sys.box_head = box_sys.box_max_idx = box_sys.box_len = 1;

    box_sys.tree_head = box_sys.tree_max_idx = box_sys.tree_len = 1;
}

// =============================================================================
u32 box_create(box **r_box) {
    if (box_sys.box_len >= box_sys.box_cap) {
        log_err("box_create(): too much boxess => stub");
        return 0;
    }

    usize idx = box_sys.box_head;
    box *bx = &box_sys.box_pool[idx];

    if (idx == box_sys.box_max_idx) {
        ++box_sys.box_max_idx;
        ++box_sys.box_head;
    } else {
        box_sys.box_head = bx->pool_flag;
    }

    ++box_sys.box_len;

    // setup box
    memset(bx, 0, sizeof(box));
    bx->pool_flag = ALIVE_POOL_FLAG;

    log_debug("Created Box [%u]", idx);

    if (r_box != nullptr) *r_box = bx;
    return idx;
}

void box_destroy(u32 idx) {
    box *bx = &box_sys.box_pool[idx];

    if (bx->pool_flag != ALIVE_POOL_FLAG) {
        log_warn("box_destroy(): box already dead");
        return;
    }

    bx->pool_flag = box_sys.box_head;
    box_sys.box_head = idx;

    --box_sys.box_len;

    // destroy from tree if is
    if (bx->tree_node != 0) {

    }

    log_debug("Destroyed box [%u]", idx);
}

[[nodiscard]] box *box_get(u32 idx) {
    if (idx == 0 || idx >= box_sys.box_max_idx) {
        log_err("box_get(): box invalid => stub");
        return &box_sys.box_pool[idx];
    }
    return &box_sys.box_pool[idx];
}

void box_add_tree(u32 idx) {
    if (idx == 0 || idx >= box_sys.box_max_idx) {
        log_err("box_add_tree(): box invalid");
        return;
    }
    if (box_sys.box_pool[idx].pool_flag != ALIVE_POOL_FLAG) {
        log_err("box_add_tree(): box is dead");
        return;
    }

    box_tree_insert(idx);
}

// =============================================================================
u32 box_node_create(u32 box_idx) {
    assert(box_sys.tree_len < box_sys.tree_cap);

    usize idx = box_sys.tree_head;
    box_node *bn = &box_sys.tree_pool[idx];

    if (idx == box_sys.tree_max_idx) {
        ++box_sys.tree_max_idx;
        ++box_sys.tree_head;
    } else {
        box_sys.tree_head = bn->pool_flag;
    }

    ++box_sys.tree_len;

    memset(bn, 0, sizeof(box));
    bn->pool_flag = ALIVE_POOL_FLAG;

    // box_idx == 0 => no linking against any actual box
    if (box_idx != 0) {
        box *bx = box_get(box_idx);
        bn->box_idx = box_idx;
        bn->x = bx->x - FAT_BOX_OFFSET;
        bn->y = bx->y - FAT_BOX_OFFSET;
        bn->w = bx->w + FAT_BOX_OFFSET + FAT_BOX_OFFSET;
        bn->h = bx->h + FAT_BOX_OFFSET + FAT_BOX_OFFSET;
        bn->flag = bx->flag;
    }

    return idx;
}
void box_node_destroy(u32 idx) {
    box_node *bn = &box_sys.tree_pool[idx];
    assert(bn->pool_flag == ALIVE_POOL_FLAG);

    bn->pool_flag = box_sys.tree_head;
    box_sys.tree_head = idx;

    --box_sys.tree_len;
}
[[nodiscard]] box_node *box_node_get(u32 idx) {
    assert(idx != 0 && idx < box_sys.tree_max_idx);
    return &box_sys.tree_pool[idx];
}

bool box_is_leaf_node(u32 idx) {
    if (box_node_get(idx)->child[0] == 0) {
        assert(box_node_get(idx)->child[1] == 0);
        return true;
    }
    return false;
}

f32 box_merged_node_area(u32 l_idx, u32 r_idx) {
    box_node l_node = *box_node_get(l_idx);
    box_node r_node = *box_node_get(r_idx);

    f32 min_x = fminf(l_node.x, r_node.x);
    f32 min_y = fminf(l_node.y, r_node.y);

    f32 max_x = fmaxf(l_node.x + l_node.w, r_node.x + r_node.w);
    f32 max_y = fmaxf(l_node.y + l_node.h, r_node.y + r_node.h);

    f32 merged_w = max_x - min_x;
    f32 merged_h = max_y - min_y;

    return merged_w * merged_h;
}

void box_merge_node(u32 dest_idx, u32 a_idx, u32 b_idx) {
    box_node *dest_node = box_node_get(dest_idx);
    box_node a_node = *box_node_get(a_idx);
    box_node b_node = *box_node_get(b_idx);

    f32 min_x = (a_node.x < b_node.x) ? a_node.x : b_node.x;
    f32 min_y = (a_node.y < b_node.y) ? a_node.y : b_node.y;

    f32 max_x = ((a_node.x + a_node.w) > (b_node.x + b_node.w)) 
                ? (a_node.x + a_node.w) 
                : (b_node.x + b_node.w);

    f32 max_y = ((a_node.y + a_node.h) > (b_node.y + b_node.h)) 
                ? (a_node.y + a_node.h) 
                : (b_node.y + b_node.h);

    dest_node->x = min_x;
    dest_node->y = min_y;
    dest_node->w = max_x - min_x;
    dest_node->h = max_y - min_y;
}

void box_tree_insert(u32 box_idx) {
    if (box_sys.tree_root == 0) {
        box_sys.tree_root = box_node_create(box_idx);
        return;
    }

    // new node
    u32 insert_idx = box_node_create(box_idx);
    box_node *insert_node = box_node_get(insert_idx);

    // get the node to insert to
    u32 cur_idx = box_sys.tree_root;
    while (true) {
        if (box_is_leaf_node(cur_idx)) break;

        box_node *cur_node = box_node_get(cur_idx);

        u32 child_idx[2] = { cur_node->child[0], cur_node->child[1] };

        box_node *child_node[2];
        child_node[0] = box_node_get(child_idx[0]);
        child_node[1] = box_node_get(child_idx[1]);

        // cost to compare to decide wheter goes does or what (math stuff)
        f32 cur_cost = cur_node->w * cur_node->h;

        f32 child_cost[2] = {0};
        child_cost[0] = box_merged_node_area(child_idx[0], insert_idx)
                        - (child_node[0]->w * child_node[0]->h);
        child_cost[1] = box_merged_node_area(child_idx[1], insert_idx)
                        - (child_node[1]->w * child_node[1]->h);

        //
        usize min_i = child_cost[0] < child_cost[1] ? 0 : 1;

        if (cur_cost < child_cost[min_i]) break;

        // rebalance tree
        if (cur_idx != box_sys.tree_root
            && child_node[min_i]->height > child_node[1 - min_i]->height) {
            // instead of rotate tree in normal way, just re-wire stuff to retain the aabb data

            u32 curpar_idx = cur_node->parent;
            assert(curpar_idx != 0);
            box_node *curpar_node = box_node_get(curpar_idx);

            usize curchild_i = curpar_node->child[0] == cur_idx ? 0 : 1;
            usize old_curchild_ops_idx = curpar_node->child[1 - curchild_i];

            curpar_node->child[0] = cur_idx;
            cur_node->parent = curpar_idx;

            curpar_node->child[1] = child_idx[min_i];
            child_node[min_i]->parent = curpar_idx;

            cur_node->child[0] = old_curchild_ops_idx;
            box_node_get(old_curchild_ops_idx)->parent = cur_idx;

            cur_node->child[1] = child_idx[1 - min_i];
            child_node[1 - min_i]->parent = cur_idx;

            // remerge some aabb
            box_merge_node(cur_idx, old_curchild_ops_idx, child_idx[1 - min_i]);
        } 

        cur_idx = child_idx[min_i];
    }

    // insert new nodes
    // common parent node of insert_node and cur_node
    u32 par_idx = box_node_create(0);
    box_node *par_node = box_node_get(par_idx);
    box_node *cur_node = box_node_get(cur_idx);

    u32 old_curpar_idx = cur_node->parent;

    par_node->child[0] = insert_idx;
    insert_node->parent = par_idx;
    
    par_node->child[1] = cur_idx;
    cur_node->parent    = par_idx;

    par_node->parent   = old_curpar_idx;
    par_node->height   = cur_node->height + 1;

    //
    if (cur_idx == box_sys.tree_root) box_sys.tree_root = par_idx;
    else {
        // rewire parent of parent
        box_node *old_curpar_node = box_node_get(old_curpar_idx);
        if (old_curpar_node->child[0] == cur_idx) {
            old_curpar_node->child[0] = par_idx;
        } else {
            old_curpar_node->child[1] = par_idx;
        }

        if (par_node->height + 1 > old_curpar_node->height) {
            old_curpar_node->height = par_node->height + 1;
        }
    }

    // update parents' aabb upward
    u32 update_idx = insert_node->parent;
    while (update_idx != 0) {
        box_node *update_node = box_node_get(update_idx);
        box_merge_node(update_idx, update_node->child[0], update_node->child[1]);
        update_idx = update_node->parent;
    }
}

// =============================================================================
void box_sys_update() {
    box_sys.box_head = box_sys.box_max_idx = box_sys.box_len = 1;

    box_sys.tree_head = box_sys.tree_max_idx = box_sys.tree_len = 1;
    box_sys.tree_root = 0;
}

void box_sys_draw_node_debug(u32 node_idx, u32 max_height) {
    box_node *node;
    float t;
    float inset;
    Rectangle rect;
    Color color;

    if (node_idx == 0) return;

    node = box_node_get(node_idx);
    if (!node) return;

    t = 0.0f;
    if (max_height > 0) {
        t = 1.0f - ((float)node->height / (float)max_height);
    }

    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    color.r = (unsigned char)(255.0f * t);
    color.g = (unsigned char)(255.0f * (1.0f - t));
    color.b = 0;
    color.a = 255;

    inset = (float)(max_height - node->height) * 2.0f;

    rect.x = node->x + inset;
    rect.y = node->y + inset;
    rect.width = node->w - (inset * 2.0f);
    rect.height = node->h - (inset * 2.0f);

    if (rect.width < 1.0f)  rect.width = 1.0f;
    if (rect.height < 1.0f) rect.height = 1.0f;

    DrawRectangleLinesEx(rect, 1.0f, color);

    box_sys_draw_node_debug(node->child[0], max_height);
    box_sys_draw_node_debug(node->child[1], max_height);
}

void box_sys_draw_debug(void) {
    if (box_sys.tree_root == 0) return;

    box_node *root = box_node_get(box_sys.tree_root);
    if (!root) return;

    // The root node's height represents the maximum depth of the tree
    u32 max_height = root->height;

    box_sys_draw_node_debug(box_sys.tree_root, max_height);
}
