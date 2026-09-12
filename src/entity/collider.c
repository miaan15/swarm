#include "collider.h"

#include "context.h"
#include "log.h"
#include <assert.h>
#include <math.h>
#include <raylib.h>

struct collider_sys collider_sys = {0};

void debug_print_collider_tree();
// =============================================================================
u32 collider_tree_insert(u32 collider_idx, bool create_new_node);
void collider_tree_remove(u32 node_idx, bool destroy_node);

// =============================================================================
void collider_sys_init(usize cap, f32 fat_aabb_offset) {
    // collider pool
    collider_sys.collider_pool = arena_alloc(&omni_arena, cap * sizeof(collider));
    collider_sys.collider_cap = cap;

    // tree
    collider_sys.tree_pool = arena_alloc(&omni_arena, (2 * cap) * sizeof(collider_node));
    collider_sys.tree_cap = 2 * cap;

    //
    collider_sys.fat_aabb_offset = fat_aabb_offset;

    // stub
    collider_sys.collider_head = collider_sys.collider_max_idx = collider_sys.collider_len = 1;

    collider_sys.tree_head = collider_sys.tree_max_idx = collider_sys.tree_len = 1;
}

// =============================================================================
u32 collider_create(collider **r_collider) {
    if (collider_sys.collider_len >= collider_sys.collider_cap) {
        log_err("collider_create(): too much collideress => stub");
        assert(false);
        return 0;
    }

    usize idx = collider_sys.collider_head;
    collider *col = &collider_sys.collider_pool[idx];

    if (idx == collider_sys.collider_max_idx) {
        ++collider_sys.collider_max_idx;
        ++collider_sys.collider_head;
    } else {
        collider_sys.collider_head = col->pool_flag;
    }

    ++collider_sys.collider_len;

    // setup collider
    memset(col, 0, sizeof(collider));
    col->pool_flag = ALIVE_POOL_FLAG;
    col->idx = idx;

    log_debug("Created Collider [%u]", idx);

    if (r_collider != nullptr) *r_collider = col;
    return idx;
}

void collider_destroy(u32 idx) {
    collider *col = &collider_sys.collider_pool[idx];

    if (col->pool_flag != ALIVE_POOL_FLAG) {
        log_warn("collider_destroy(): collider already dead");
        return;
    }

    col->pool_flag = collider_sys.collider_head;
    collider_sys.collider_head = idx;

    --collider_sys.collider_len;

    // destroy from tree if is
    if (col->tree_node_idx != 0) {
        collider_tree_remove(col->tree_node_idx, true);
    }

    log_debug("Destroyed Collider [%u]", idx);
}

[[nodiscard]] collider *collider_get(u32 idx) {
    if (idx == 0 || idx >= collider_sys.collider_max_idx) {
        log_err("collider_get(): collider invalid => stub");
        assert(false);
        return &collider_sys.collider_pool[idx];
    }
    return &collider_sys.collider_pool[idx];
}

void collider_add_to_tree(u32 idx) {
    if (idx == 0 || idx >= collider_sys.collider_max_idx) {
        log_err("collider_add_tree(): collider invalid");
        assert(false);
        return;
    }
    if (collider_sys.collider_pool[idx].pool_flag != ALIVE_POOL_FLAG) {
        log_err("collider_add_tree(): collider is dead");
        assert(false);
        return;
    }

    collider_get(idx)->tree_node_idx = collider_tree_insert(idx, true);
    // log_info(">>>>> %u", collider_get(idx)->tree_node_idx);
}

// =============================================================================
u32 collider_node_create(u32 collider_idx) {
    assert(collider_sys.tree_len < collider_sys.tree_cap);

    usize node_idx = collider_sys.tree_head;
    collider_node *col_node = &collider_sys.tree_pool[node_idx];

    if (node_idx == collider_sys.tree_max_idx) {
        ++collider_sys.tree_max_idx;
        ++collider_sys.tree_head;
    } else {
        collider_sys.tree_head = col_node->pool_flag;
    }

    ++collider_sys.tree_len;

    memset(col_node, 0, sizeof(collider_node));
    col_node->pool_flag = ALIVE_POOL_FLAG;
    col_node->pool_idx = node_idx;

    // collider_idx == 0 => no linking against any actual collider
    if (collider_idx != 0) {
        collider *col = collider_get(collider_idx);
        col_node->collider_idx = collider_idx;
        col_node->x = col->x - collider_sys.fat_aabb_offset;
        col_node->y = col->y - collider_sys.fat_aabb_offset;
        col_node->w = col->w + collider_sys.fat_aabb_offset + collider_sys.fat_aabb_offset;
        col_node->h = col->h + collider_sys.fat_aabb_offset + collider_sys.fat_aabb_offset;
        col_node->flag = col->flag;
    }

    return node_idx;
}

void collider_node_destroy(u32 idx) {
    collider_node *col_node = &collider_sys.tree_pool[idx];
    assert(col_node->pool_flag == ALIVE_POOL_FLAG);

    col_node->pool_flag = collider_sys.tree_head;
    collider_sys.tree_head = idx;

    --collider_sys.tree_len;
}

void collider_node_update(u32 collider_idx) {
    collider *col = collider_get(collider_idx);
    collider_node *col_node = &collider_sys.tree_pool[col->tree_node_idx];
    col_node->collider_idx = collider_idx;
    col_node->x = col->x - collider_sys.fat_aabb_offset;
    col_node->y = col->y - collider_sys.fat_aabb_offset;
    col_node->w = col->w + collider_sys.fat_aabb_offset + collider_sys.fat_aabb_offset;
    col_node->h = col->h + collider_sys.fat_aabb_offset + collider_sys.fat_aabb_offset;
    col_node->flag = col->flag;
}

[[nodiscard]] collider_node *collider_node_get(u32 idx) {
    assert(idx != 0 && idx < collider_sys.tree_max_idx);
    return &collider_sys.tree_pool[idx];
}

bool collider_node_is_leaf(u32 idx) {
    if (collider_node_get(idx)->child[0] == 0) {
        assert(collider_node_get(idx)->child[1] == 0);
        return true;
    }
    return false;
}

f32 collider_aabb_merged_node_area(u32 l_idx, u32 r_idx) {
    collider_node l_node = *collider_node_get(l_idx);
    collider_node r_node = *collider_node_get(r_idx);

    f32 min_x = fminf(l_node.x, r_node.x);
    f32 min_y = fminf(l_node.y, r_node.y);

    f32 max_x = fmaxf(l_node.x + l_node.w, r_node.x + r_node.w);
    f32 max_y = fmaxf(l_node.y + l_node.h, r_node.y + r_node.h);

    f32 merged_w = max_x - min_x;
    f32 merged_h = max_y - min_y;

    return merged_w * merged_h;
}

void collider_aabb_merge_node(u32 dest_idx, u32 a_idx, u32 b_idx) {
    collider_node *dest_node = collider_node_get(dest_idx);
    collider_node a_node = *collider_node_get(a_idx);
    collider_node b_node = *collider_node_get(b_idx);

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

void collider_node_recal_aabb_height(u32 node_idx) {
    if (collider_node_is_leaf(node_idx)) return;

    collider_node *node = collider_node_get(node_idx);

    u32 child_idx[2] = { node->child[0], node->child[1] };
    collider_node *child_node[2] = { collider_node_get(child_idx[0]), collider_node_get(child_idx[1]) };

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

u32 collider_tree_insert(u32 collider_idx, bool create_new_node) {
    if (collider_sys.tree_root == 0) {
        collider_sys.tree_root = collider_node_create(collider_idx);
        return collider_sys.tree_root;
    }

    // new node
    u32 insert_idx = create_new_node ? collider_node_create(collider_idx) : collider_get(collider_idx)->tree_node_idx;
    if (!create_new_node) collider_node_update(collider_idx);
    collider_node *insert_node = collider_node_get(insert_idx);

    // get the node to insert to and rebalance tree
    u32 cur_idx = collider_sys.tree_root;
    while (true) {
        if (collider_node_is_leaf(cur_idx)) break;

        collider_node *cur_node = collider_node_get(cur_idx);

        u32 child_idx[2] = { cur_node->child[0], cur_node->child[1] };

        collider_node *child_node[2] = { collider_node_get(child_idx[0]), collider_node_get(child_idx[1]) };

        // cost to compare to decide wheter goes does or what (math stuff)
        f32 cur_cost = cur_node->w * cur_node->h;

        f32 child_cost[2] = {0};
        child_cost[0] = collider_aabb_merged_node_area(child_idx[0], insert_idx)
                        - (child_node[0]->w * child_node[0]->h);
        child_cost[1] = collider_aabb_merged_node_area(child_idx[1], insert_idx)
                        - (child_node[1]->w * child_node[1]->h);

        //
        usize min_i = child_cost[0] < child_cost[1] ? 0 : 1;

        if (cur_cost < child_cost[min_i]) break;

        // other heuristic: use perimeter?, idk, maybe it's better?
        min_i = child_node[0]->w + child_node[0]->h < child_node[1]->w + child_node[1]->h ? 0 : 1;

        // rebalance tree
        // instead of rotate tree in normal way, just re-wire stuff to retain the aabb data
        if (cur_idx != collider_sys.tree_root
            && child_node[min_i]->height > child_node[1 - min_i]->height) {

            u32 curpar_idx = cur_node->parent;
            assert(curpar_idx != 0);
            collider_node *curpar_node = collider_node_get(curpar_idx);

            usize cur_child_i = curpar_node->child[0] == cur_idx ? 0 : 1;
            usize _cur_sib_idx = curpar_node->child[1 - cur_child_i];

            curpar_node->child[0] = cur_idx;
            cur_node->parent = curpar_idx;

            curpar_node->child[1] = child_idx[min_i];
            child_node[min_i]->parent = curpar_idx;

            cur_node->child[0] = child_idx[1 - min_i];
            child_node[1 - min_i]->parent = cur_idx;

            cur_node->child[1] = _cur_sib_idx;
            collider_node_get(_cur_sib_idx)->parent = cur_idx;


            // remerge some aabb, height
            collider_node_recal_aabb_height(cur_idx);
        }

        cur_idx = child_idx[min_i];
    }

    // insert new nodes
    // common parent node of insert_node and cur_node
    u32 par_idx = collider_node_create(0);
    collider_node *par_node = collider_node_get(par_idx);
    collider_node *cur_node = collider_node_get(cur_idx);

    u32 parpar_idx = cur_node->parent;

    par_node->child[0] = insert_idx;
    insert_node->parent = par_idx;

    par_node->child[1] = cur_idx;
    cur_node->parent = par_idx;

    par_node->parent = parpar_idx;
    par_node->height = cur_node->height + 1;

    if (parpar_idx == 0) { // par_node is new tree_root
        collider_sys.tree_root = par_idx;
        par_node->parent = 0;
    }
    else {
        // rewire parent of parent
        collider_node *parpar_node = collider_node_get(parpar_idx);
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
        collider_node_recal_aabb_height(ud_idx);
        ud_idx = collider_node_get(ud_idx)->parent;
    }

    // log_info("tree after INSERT node[%u]", insert_idx);
    // debug_print_collider_tree();
    // printf("\n");

    return insert_idx;
}

void collider_tree_remove(u32 node_idx, bool destroy_node) {
    assert(collider_node_is_leaf(node_idx));

    if (node_idx == collider_sys.tree_root) {
        collider_node_destroy(node_idx);
        collider_sys.tree_root = 0;
        return;
    }

    u32 cur_idx; // saved for later

    // destroy actual node
    {
        collider_node *remv_node = collider_node_get(node_idx);

        u32 par_idx = remv_node->parent;
        collider_node *par_node = collider_node_get(par_idx);

        u32 parpar_idx = par_node->parent;
        if (parpar_idx == 0) { // if there's only total 2 leaf node => tree back to 1 node only
            collider_sys.tree_root = par_node->child[0] != node_idx
                                ? par_node->child[0]
                                : par_node->child[1];
            collider_node_get(collider_sys.tree_root)->parent = 0;

            collider_node_destroy(node_idx);
            collider_node_destroy(par_idx);
            return;
        }
        collider_node *parpar_node = collider_node_get(parpar_idx);

        u32 remv_sib_idx = par_node->child[0] == node_idx ? par_node->child[1] : par_node->child[0];
        collider_node *remv_sib_node = collider_node_get(remv_sib_idx);

        usize par_child_i = parpar_node->child[0] == par_idx ? 0 : 1;

        parpar_node->child[par_child_i] = remv_sib_idx;
        remv_sib_node->parent = parpar_idx;

        collider_node_recal_aabb_height(parpar_idx);

        if (destroy_node) collider_node_destroy(node_idx);
        collider_node_destroy(par_idx);

        cur_idx = remv_sib_idx;
    }
    //
    // log_info("tree just-destroy-stuff REMOVE [%u]", node_idx);
    // debug_print_collider_tree();
    // printf("\n");
    //
    // update parents' aabb upward and rebalance tree
    while (cur_idx != 0) {
        if (cur_idx == collider_sys.tree_root) {
            collider_sys.tree_root = cur_idx;
            break;
        }

        collider_node *cur_node = collider_node_get(cur_idx);

        u32 curpar_idx = cur_node->parent;
        // NOTE: well, apparently, no tree rotate in delete make better performance

        // collider_node *curpar_node = collider_node_get(curpar_idx);

        // u32 cur_child_i = curpar_node->child[0] == cur_idx ? 0 : 1;

        // u32 cur_sib_idx = curpar_node->child[1 - cur_child_i];
        // collider_node *cur_sib_node = collider_node_get(cur_sib_idx);

        // rebalance tree
        // instead of rotate tree in normal way, just re-wire stuff to retain the aabb data
        // if (cur_idx != collider_sys.tree_root
        //     && cur_sib_node->height > cur_node->height) {
        //
        //     u32 _cur_sib_child_idx[2] = { cur_sib_node->child[0], cur_sib_node->child[1] };
        //     collider_node *_cur_sib_child_node[2] = { collider_node_get(_cur_sib_child_idx[0]),
        //                                          collider_node_get(_cur_sib_child_idx[1]) };
        //     u32 cur_sib_child_max_i = _cur_sib_child_node[0]->height > _cur_sib_child_node[1]->height ? 0 : 1;
        //
        //     curpar_node->child[0] = cur_sib_idx;
        //     cur_sib_node->parent = curpar_idx;
        //
        //     curpar_node->child[1] = _cur_sib_child_idx[cur_sib_child_max_i];
        //     _cur_sib_child_node[cur_sib_child_max_i]->parent = curpar_idx;
        //
        //     cur_sib_node->child[0] = _cur_sib_child_idx[1 - cur_sib_child_max_i];
        //     _cur_sib_child_node[1 - cur_sib_child_max_i]->parent = cur_sib_idx;
        //
        //     cur_sib_node->child[1] = cur_idx;
        //     cur_node->parent = cur_sib_idx;
        //
        //     // remerge some aabb, height
        //     collider_node_recal_aabb_height(cur_sib_idx);
        // }

        cur_idx = curpar_idx;

        // update parent aabb, height
        if (cur_idx != 0) { // if now the tree only have one
            collider_node_recal_aabb_height(cur_idx);
        }
    }

    // log_info("tree after REMOVE [%u]", node_idx);
    // debug_print_collider_tree();
    // printf("\n");
}

// =============================================================================
void collider_sys_update() {
    for (usize i = 1; i < collider_sys.collider_max_idx; ++i) {
        collider *col = collider_get(i);
        if (col->pool_flag != ALIVE_POOL_FLAG) continue;

        if (col->tree_node_idx != 0) {
            collider_node *col_node = collider_node_get(col->tree_node_idx);
            if (col->x < col_node->x
                || col->y < col_node->y
                || col->x + col->w > col_node->x + col_node->w
                || col->y + col->h > col_node->y + col_node->h) {
                collider_tree_remove(col->tree_node_idx, false);
                collider_tree_insert(i, false);
            }
        }
    }
}

void collider_sys_draw_node_debug(u32 node_idx, u32 max_height) {
    collider_node *node;
    Rectangle rect;
    Color color;

    if (node_idx == 0) return;

    node = collider_node_get(node_idx);
    if (!node) return;

    color.r = 255;
    color.g = 200;
    color.b = 200;
    color.a = 50;

    rect.x = node->x;
    rect.y = node->y;
    rect.width = node->w;
    rect.height = node->h;

    if (rect.width < 1.0f)  rect.width = 1.0f;
    if (rect.height < 1.0f) rect.height = 1.0f;

    DrawRectangleLinesEx(rect, 1.0f, color);

    if (node->collider_idx != 0) {
        collider *col = collider_get(node->collider_idx);
        rect.x = col->x;
        rect.y = col->y;
        rect.width = col->w;
        rect.height = col->h;
        DrawRectangleLinesEx(rect, 2.0f, GREEN);
    }

    collider_sys_draw_node_debug(node->child[0], max_height);
    collider_sys_draw_node_debug(node->child[1], max_height);
}

void collider_sys_draw_debug(void) {
    if (collider_sys.tree_root == 0) return;

    collider_node *root = collider_node_get(collider_sys.tree_root);
    if (!root) return;

    u32 max_height = root->height;

    collider_sys_draw_node_debug(collider_sys.tree_root, max_height);
}

void print_collider_tree_recursive(u32 idx, char *prefix, bool is_left, bool is_root) {
    if (idx == 0) return;

    collider_node *node = collider_node_get(idx);
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
        print_collider_tree_recursive(left, next_prefix, true, false);
        print_collider_tree_recursive(right, next_prefix, false, false);
    } else if (left != 0) {
        print_collider_tree_recursive(left, next_prefix, false, false);
    } else if (right != 0) {
        print_collider_tree_recursive(right, next_prefix, false, false);
    }
}

void debug_print_collider_tree() {
    if (collider_sys.tree_root == 0) {
        printf("(empty tree)\n");
        return;
    }
    print_collider_tree_recursive(collider_sys.tree_root, "", false, true);
}
