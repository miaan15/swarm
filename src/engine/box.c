#include "box.h"

#include "context.h"
#include "log.h"
#include <assert.h>
#include <math.h>
#include <raylib.h>

struct box_sys box_sys = {0};

void debug_print_box_tree();
// =============================================================================
u32 box_tree_insert(u32 box_idx, bool create_new_node);
void box_tree_remove(u32 node_idx, bool destroy_node);

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
    if (bx->tree_node_idx != 0) {
        box_tree_remove(bx->tree_node_idx, true);
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

void box_add_to_tree(u32 idx) {
    if (idx == 0 || idx >= box_sys.box_max_idx) {
        log_err("box_add_tree(): box invalid");
        return;
    }
    if (box_sys.box_pool[idx].pool_flag != ALIVE_POOL_FLAG) {
        log_err("box_add_tree(): box is dead");
        return;
    }

    box_get(idx)->tree_node_idx = box_tree_insert(idx, true);
    log_info(">>>>> %u", box_get(idx)->tree_node_idx);
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

void box_node_update(u32 box_idx) {
    box *bx = box_get(box_idx);
    box_node *bn = &box_sys.tree_pool[bx->tree_node_idx];
    bn->box_idx = box_idx;
    bn->x = bx->x - FAT_BOX_OFFSET;
    bn->y = bx->y - FAT_BOX_OFFSET;
    bn->w = bx->w + FAT_BOX_OFFSET + FAT_BOX_OFFSET;
    bn->h = bx->h + FAT_BOX_OFFSET + FAT_BOX_OFFSET;
    bn->flag = bx->flag;
}

[[nodiscard]] box_node *box_node_get(u32 idx) {
    assert(idx != 0 && idx < box_sys.tree_max_idx);
    return &box_sys.tree_pool[idx];
}

bool box_node_is_leaf(u32 idx) {
    if (box_node_get(idx)->child[0] == 0) {
        assert(box_node_get(idx)->child[1] == 0);
        return true;
    }
    return false;
}

f32 box_aabb_merged_node_area(u32 l_idx, u32 r_idx) {
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

void box_aabb_merge_node(u32 dest_idx, u32 a_idx, u32 b_idx) {
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

void box_node_recal_aabb_height(u32 node_idx) {
    if (box_node_is_leaf(node_idx)) return;

    box_node *node = box_node_get(node_idx);

    u32 child_idx[2] = { node->child[0], node->child[1] };
    box_node *child_node[2] = { box_node_get(child_idx[0]), box_node_get(child_idx[1]) };

    f32 min_x = (child_node[0]->x < child_node[1]->x) ? child_node[0]->x : child_node[1]->x;
    f32 min_y = (child_node[0]->y < child_node[1]->y) ? child_node[0]->y : child_node[1]->y;

    f32 max_x = ((child_node[0]->x + child_node[0]->w) > (child_node[1]->x + child_node[1]->w))
                ? (child_node[0]->x + child_node[0]->w)
                : (child_node[1]->x + child_node[1]->w);

    f32 max_y = ((child_node[0]->y + child_node[0]->h) > (child_node[1]->y + child_node[1]->h))
                ? (child_node[0]->y + child_node[0]->h)
                : (child_node[1]->y + child_node[1]->h);

    node->x = min_x;
    node->y = min_y;
    node->w = max_x - min_x;
    node->h = max_y - min_y;

    node->height = child_node[0]->height > child_node[1]->height
                   ? child_node[0]->height + 1
                   : child_node[1]->height + 1;
}

u32 box_tree_insert(u32 box_idx, bool create_new_node) {
    if (box_sys.tree_root == 0) {
        box_sys.tree_root = box_node_create(box_idx);
        return box_sys.tree_root;
    }

    // new node
    u32 insert_idx = create_new_node ? box_node_create(box_idx) : box_get(box_idx)->tree_node_idx;
    if (!create_new_node) box_node_update(box_idx);
    box_node *insert_node = box_node_get(insert_idx);

    // get the node to insert to and rebalance tree
    u32 cur_idx = box_sys.tree_root;
    while (true) {
        if (box_node_is_leaf(cur_idx)) break;

        box_node *cur_node = box_node_get(cur_idx);

        u32 child_idx[2] = { cur_node->child[0], cur_node->child[1] };

        box_node *child_node[2];
        child_node[0] = box_node_get(child_idx[0]);
        child_node[1] = box_node_get(child_idx[1]);

        // cost to compare to decide wheter goes does or what (math stuff)
        f32 cur_cost = cur_node->w * cur_node->h;

        f32 child_cost[2] = {0};
        child_cost[0] = box_aabb_merged_node_area(child_idx[0], insert_idx)
                        - (child_node[0]->w * child_node[0]->h);
        child_cost[1] = box_aabb_merged_node_area(child_idx[1], insert_idx)
                        - (child_node[1]->w * child_node[1]->h);

        //
        usize min_i = child_cost[0] < child_cost[1] ? 0 : 1;

        if (cur_cost < child_cost[min_i]) break;

        // rebalance tree
        // instead of rotate tree in normal way, just re-wire stuff to retain the aabb data
        if (cur_idx != box_sys.tree_root
            && child_node[min_i]->height > child_node[1 - min_i]->height) {

            u32 curpar_idx = cur_node->parent;
            assert(curpar_idx != 0);
            box_node *curpar_node = box_node_get(curpar_idx);

            usize cur_child_i = curpar_node->child[0] == cur_idx ? 0 : 1;
            usize _cur_sib_idx = curpar_node->child[1 - cur_child_i];

            curpar_node->child[0] = cur_idx;
            cur_node->parent = curpar_idx;

            curpar_node->child[1] = child_idx[min_i];
            child_node[min_i]->parent = curpar_idx;

            cur_node->child[0] = _cur_sib_idx;
            box_node_get(_cur_sib_idx)->parent = cur_idx;

            cur_node->child[1] = child_idx[1 - min_i];
            child_node[1 - min_i]->parent = cur_idx;

            // remerge some aabb, height
            box_node_recal_aabb_height(cur_idx);
        }

        cur_idx = child_idx[min_i];
    }

    // insert new nodes
    // common parent node of insert_node and cur_node
    u32 par_idx = box_node_create(0);
    box_node *par_node = box_node_get(par_idx);
    box_node *cur_node = box_node_get(cur_idx);

    u32 parpar_idx = cur_node->parent;

    par_node->child[0] = insert_idx;
    insert_node->parent = par_idx;

    par_node->child[1] = cur_idx;
    cur_node->parent = par_idx;

    par_node->parent = parpar_idx;
    par_node->height = cur_node->height + 1;

    if (parpar_idx == 0) { // par_node is new tree_root
        box_sys.tree_root = par_idx;
    }
    else {
        // rewire parent of parent
        box_node *parpar_node = box_node_get(parpar_idx);
        if (parpar_node->child[0] == cur_idx) {
            parpar_node->child[0] = par_idx;
        } else {
            parpar_node->child[1] = par_idx;
        }

        if (par_node->height + 1 > parpar_node->height) {
            parpar_node->height = par_node->height + 1;
        }
    }

    // update parents' aabb upward
    u32 ud_idx = insert_node->parent;
    while (ud_idx != 0) {
        box_node_recal_aabb_height(ud_idx);
        ud_idx = box_node_get(ud_idx)->parent;
    }

    log_info("tree after INSERT node[%u]", insert_idx);
    debug_print_box_tree();
    printf("\n");

    return insert_idx;
}

void box_tree_remove(u32 node_idx, bool destroy_node) {
    assert(box_node_is_leaf(node_idx));

    if (node_idx == box_sys.tree_root) {
        box_node_destroy(node_idx);
        box_sys.tree_root = 0;
        return;
    }

    u32 cur_idx; // saved for later

    // destroy actual node
    {
        box_node *remv_node = box_node_get(node_idx);

        u32 par_idx = remv_node->parent;
        box_node *par_node = box_node_get(par_idx);

        u32 parpar_idx = par_node->parent;
        if (parpar_idx == 0) { // if there's only total 2 leaf node => tree back to 1 node only
            box_sys.tree_root = par_node->child[0] != node_idx
                                ? par_node->child[0]
                                : par_node->child[1];
            box_node_destroy(node_idx);
            box_node_destroy(par_idx);
            return;
        }
        box_node *parpar_node = box_node_get(parpar_idx);

        u32 remv_sib_idx = par_node->child[0] == node_idx ? par_node->child[1] : par_node->child[0];
        box_node *remv_sib_node = box_node_get(remv_sib_idx);

        usize par_child_i = parpar_node->child[0] == par_idx ? 0 : 1;

        parpar_node->child[par_child_i] = remv_sib_idx;
        remv_sib_node->parent = parpar_idx;

        box_node_recal_aabb_height(parpar_idx);

        if (destroy_node) box_node_destroy(node_idx);
        box_node_destroy(par_idx);

        cur_idx = remv_sib_idx;
    }
    //
    // log_info("tree just-destroy-stuff REMOVE [%u]", node_idx);
    // debug_print_box_tree();
    // printf("\n");
    //
    // update parents' aabb upward and rebalance tree
    while (cur_idx != 0) {
        if (cur_idx == box_sys.tree_root) {
            box_sys.tree_root = cur_idx;
            break;
        }

        box_node *cur_node = box_node_get(cur_idx);

        u32 curpar_idx = cur_node->parent;
        box_node *curpar_node = box_node_get(curpar_idx);

        u32 cur_child_i = curpar_node->child[0] == cur_idx ? 0 : 1;

        u32 cur_sib_idx = curpar_node->child[1 - cur_child_i];
        box_node *cur_sib_node = box_node_get(cur_sib_idx);

        // rebalance tree
        // instead of rotate tree in normal way, just re-wire stuff to retain the aabb data
        if (cur_idx != box_sys.tree_root
            && cur_sib_node->height > cur_node->height) {

            u32 _cur_sib_child_idx[2] = { cur_sib_node->child[0], cur_sib_node->child[1] };
            box_node *_cur_sib_child_node[2] = { box_node_get(_cur_sib_child_idx[0]),
                                                 box_node_get(_cur_sib_child_idx[1]) };
            u32 cur_sib_child_max_i = _cur_sib_child_node[0]->height > _cur_sib_child_node[1]->height ? 0 : 1;

            curpar_node->child[0] = _cur_sib_child_idx[cur_sib_child_max_i];
            _cur_sib_child_node[cur_sib_child_max_i]->parent = curpar_idx;

            curpar_node->child[1] = cur_sib_idx;
            cur_sib_node->parent = curpar_idx;

            cur_sib_node->child[0] = _cur_sib_child_idx[1 - cur_sib_child_max_i];
            _cur_sib_child_node[1 - cur_sib_child_max_i]->parent = cur_sib_idx;

            cur_sib_node->child[1] = cur_idx;
            cur_node->parent = cur_sib_idx;

            // remerge some aabb, height
            box_node_recal_aabb_height(cur_sib_idx);
        }

        cur_idx = curpar_idx;

        // update parent aabb, height
        if (cur_idx != 0) { // if now the tree only have one
            box_node_recal_aabb_height(cur_idx);
        }
    }

    log_info("tree after REMOVE [%u]", node_idx);
    debug_print_box_tree();
    printf("\n");
}

// =============================================================================
void box_sys_update() {
    for (usize i = 1; i < box_sys.box_max_idx; ++i) {
        box *bx = box_get(i);
        if (bx->pool_flag != ALIVE_POOL_FLAG) continue;

        if (bx->tree_node_idx != 0) {
            box_tree_remove(bx->tree_node_idx, false);
            box_tree_insert(i, false);
        }
    }
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


void print_box_tree_recursive(u32 idx, char *prefix, bool is_left, bool is_root) {
    if (idx == 0) return;

    box_node *node = box_node_get(idx);
    if (!node) return;

    // Print current line prefix and branch connector
    printf("%s", prefix);
    if (!is_root) {
        printf("%s", is_left ? "├── " : "└── ");
    }
    printf("%u\n", idx);

    // Compute prefix for child subtrees
    char next_prefix[256];
    if (is_root) {
        next_prefix[0] = '\0';
    } else {
        snprintf(next_prefix, sizeof(next_prefix), "%s%s", prefix, is_left ? "│   " : "    ");
    }

    u32 left = node->child[0];
    u32 right = node->child[1];

    if (left != 0 && right != 0) {
        print_box_tree_recursive(left, next_prefix, true, false);
        print_box_tree_recursive(right, next_prefix, false, false);
    } else if (left != 0) {
        print_box_tree_recursive(left, next_prefix, false, false);
    } else if (right != 0) {
        print_box_tree_recursive(right, next_prefix, false, false);
    }
}

void debug_print_box_tree() {
    if (box_sys.tree_root == 0) {
        printf("(empty tree)\n");
        return;
    }
    print_box_tree_recursive(box_sys.tree_root, "", false, true);
}
