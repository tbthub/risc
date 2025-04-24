#include "lib/rbtree.h"

#include "std/stddef.h"
#include "std/stdio.h"

#define rb_set_red(node)                                                                                                                                                                               \
    do {                                                                                                                                                                                               \
        (node)->rb_parent_color &= ~1;                                                                                                                                                                 \
    } while (0)
#define rb_set_black(node)                                                                                                                                                                             \
    do {                                                                                                                                                                                               \
        (node)->rb_parent_color |= 1;                                                                                                                                                                  \
    } while (0)
#define rb_set_color(n, c)                                                                                                                                                                             \
    do {                                                                                                                                                                                               \
        (n)->rb_parent_color = ((n)->rb_parent_color & ~1) | (c);                                                                                                                                      \
    } while (0)

/* 父节点操作宏 */
#define rb_set_parent(r, p)                                                                                                                                                                            \
    do {                                                                                                                                                                                               \
        (r)->rb_parent_color = ((unsigned long)(p) & ~3UL) | ((r)->rb_parent_color & 3);                                                                                                               \
    } while (0)

/*---------------------- 旋转操作 ----------------------*/
static void rb_rotate_left(struct rb_node *node, struct rb_root *root)
{
    struct rb_node *right = node->rb_right;
    struct rb_node *parent = rb_parent(node);

    node->rb_right = right->rb_left;
    if (right->rb_left)
        rb_set_parent(right->rb_left, node);  // 修复子节点的父指针

    right->rb_left = node;
    rb_set_parent(right, parent);

    if (parent) {
        if (node == parent->rb_left)
            parent->rb_left = right;
        else
            parent->rb_right = right;
    }
    else {
        root->rb_node = right;
    }
    rb_set_parent(node, right);  // 确保原节点的父指针更新
}

static void rb_rotate_right(struct rb_node *node, struct rb_root *root)
{
    struct rb_node *left = node->rb_left;
    struct rb_node *parent = rb_parent(node);

    /* 处理左子节点的右子树 */
    if ((node->rb_left = left->rb_right)) {
        rb_set_parent(left->rb_right, node);
    }
    left->rb_right = node;

    /* 更新父指针 */
    rb_set_parent(left, parent);
    rb_set_parent(node, left);

    /* 连接父节点 */
    if (parent) {
        if (node == parent->rb_right)
            parent->rb_right = left;
        else
            parent->rb_left = left;
    }
    else {
        root->rb_node = left;
    }
}

/*---------------------- 插入操作 ----------------------*/
static void rb_insert_fixup(struct rb_node *node, struct rb_root *root)
{
    struct rb_node *parent, *gparent;

    while ((parent = rb_parent(node))) {
        if (rb_is_black(parent))
            break;

        gparent = rb_parent(parent);
        if (!gparent)
            break;

        if (parent == gparent->rb_left) {
            struct rb_node *uncle = gparent->rb_right;
            if (uncle && rb_is_red(uncle)) {
                rb_set_black(parent);
                rb_set_black(uncle);
                rb_set_red(gparent);
                node = gparent;
                continue;
            }

            if (node == parent->rb_right) {
                node = parent;
                rb_rotate_left(node, root);
                parent = rb_parent(node);
                gparent = parent ? rb_parent(parent) : NULL;
            }

            if (parent && gparent) {
                rb_set_black(parent);
                rb_set_red(gparent);
                rb_rotate_right(gparent, root);
            }
        }
        else {
            struct rb_node *uncle = gparent->rb_left;
            if (uncle && rb_is_red(uncle)) {
                rb_set_black(parent);
                rb_set_black(uncle);
                rb_set_red(gparent);
                node = gparent;
                continue;
            }

            if (node == parent->rb_left) {
                node = parent;
                rb_rotate_right(node, root);
                parent = rb_parent(node);
                gparent = parent ? rb_parent(parent) : NULL;
            }

            if (parent && gparent) {
                rb_set_black(parent);
                rb_set_red(gparent);
                rb_rotate_left(gparent, root);
            }
        }
    }
    rb_set_black(root->rb_node);
}

void rb_insert(struct rb_node *node, struct rb_root *root, int (*cmp)(struct rb_node *, struct rb_node *))
{
    struct rb_node **link = &root->rb_node;
    struct rb_node *parent = NULL;

    /* 查找插入位置 */
    while (*link) {
        parent = *link;
        if (cmp(node, parent) < 0)
            link = &parent->rb_left;
        else
            link = &parent->rb_right;
    }

    /* 初始化新节点 */
    rb_set_parent(node, parent);
    node->rb_left = node->rb_right = NULL;
    rb_set_red(node);

    /* 插入节点 */
    *link = node;

    /* 修复红黑树性质 */
    rb_insert_fixup(node, root);
}

static void rb_erase_fixup(struct rb_node *node, struct rb_node *parent, struct rb_root *root)
{
    struct rb_node *other;

    while (node != root->rb_node && (!node || rb_is_black(node))) {
        if (parent->rb_left == node) {
            other = parent->rb_right;
            if (!other)
                break;

            if (rb_is_red(other)) {
                rb_set_black(other);
                rb_set_red(parent);
                rb_rotate_left(parent, root);
                other = parent->rb_right;
                if (!other)
                    break;
            }

            if ((!other->rb_left || rb_is_black(other->rb_left)) && (!other->rb_right || rb_is_black(other->rb_right))) {
                rb_set_red(other);
                node = parent;
                parent = rb_parent(node);
            }
            else {
                if (!other->rb_right || rb_is_black(other->rb_right)) {
                    if (other->rb_left)
                        rb_set_black(other->rb_left);
                    rb_set_red(other);
                    rb_rotate_right(other, root);
                    other = parent->rb_right;
                }
                rb_set_color(other, rb_color(parent));
                rb_set_black(parent);
                if (other->rb_right)
                    rb_set_black(other->rb_right);
                rb_rotate_left(parent, root);
                node = root->rb_node;
                break;
            }
        }
        else {
            // 对称处理右侧情况
            other = parent->rb_left;
            if (!other)
                break;

            if (rb_is_red(other)) {
                rb_set_black(other);
                rb_set_red(parent);
                rb_rotate_right(parent, root);
                other = parent->rb_left;
                if (!other)
                    break;
            }

            if ((!other->rb_left || rb_is_black(other->rb_left)) && (!other->rb_right || rb_is_black(other->rb_right))) {
                rb_set_red(other);
                node = parent;
                parent = rb_parent(node);
            }
            else {
                if (!other->rb_left || rb_is_black(other->rb_left)) {
                    if (other->rb_right)
                        rb_set_black(other->rb_right);
                    rb_set_red(other);
                    rb_rotate_left(other, root);
                    other = parent->rb_left;
                }
                rb_set_color(other, rb_color(parent));
                rb_set_black(parent);
                if (other->rb_left)
                    rb_set_black(other->rb_left);
                rb_rotate_right(parent, root);
                node = root->rb_node;
                break;
            }
        }
    }
    if (node)
        rb_set_black(node);
}

void rb_erase(struct rb_node *node, struct rb_root *root)
{
    struct rb_node *child, *parent;
    int color;

    if (!node->rb_left) {
        child = node->rb_right;
    }
    else if (!node->rb_right) {
        child = node->rb_left;
    }
    else {
        struct rb_node *old = node;
        struct rb_node *left;
        int original_color;

        node = old->rb_right;
        while ((left = node->rb_left) != NULL)
            node = left;

        original_color = rb_color(node);
        child = node->rb_right;
        parent = rb_parent(node);

        if (child)
            rb_set_parent(child, parent);

        if (node != old->rb_right) {
            parent->rb_left = child;
            node->rb_right = old->rb_right;
            rb_set_parent(old->rb_right, node);
        }

        if (node != old->rb_right) {
            parent->rb_left = child;
            if (child)
                rb_set_parent(child, parent);
            node->rb_right = old->rb_right;
            rb_set_parent(old->rb_right, node);
        }
        else {
            if (child)
                rb_set_parent(child, node);  // 设置child的父指针为当前节点
            parent = rb_parent(node);
        }

        node->rb_parent_color = old->rb_parent_color;
        node->rb_left = old->rb_left;
        rb_set_parent(old->rb_left, node);

        if (rb_parent(old)) {
            if (rb_parent(old)->rb_left == old)
                rb_parent(old)->rb_left = node;
            else
                rb_parent(old)->rb_right = node;
        }
        else {
            root->rb_node = node;
        }

        old->rb_left = NULL;
        old->rb_right = NULL;

        color = original_color;
        goto fix_color;
    }

    parent = rb_parent(node);
    color = rb_color(node);

    if (child)
        rb_set_parent(child, parent);

    if (parent) {
        if (parent->rb_left == node)
            parent->rb_left = child;
        else
            parent->rb_right = child;
    }
    else {
        root->rb_node = child;
    }

fix_color:
    if (color == RBT_BLACK)
        rb_erase_fixup(child, parent, root);
}

struct rb_node *rb_search(struct rb_root *root, void *key, int (*cmp)(struct rb_node *, void *))
{
    struct rb_node *node = root->rb_node;

    while (node) {
        int ret = cmp(node, key);
        if (ret > 0)
            node = node->rb_right;
        else if (ret < 0)
            node = node->rb_left;
        else
            return node;
    }
    return NULL;
}

struct rb_node *rb_first(struct rb_root *root)
{
    struct rb_node *n = root->rb_node;
    if (!n)
        return NULL;
    while (n->rb_left)
        n = n->rb_left;
    return n;
}

struct rb_node *rb_next(struct rb_node *node)
{
    /* 沿着右子树找最小节点 */
    if (node->rb_right) {
        node = node->rb_right;
        while (node->rb_left)
            node = node->rb_left;
        return node;
    }

    /* 向上追溯第一个左祖先 */
    struct rb_node *parent;
    while ((parent = rb_parent(node))) {
        if (node == parent->rb_left)
            return parent;
        node = parent;
    }
    return NULL;  // 到达根节点且没有更高值的祖先
}

#ifdef RBT_DEBUG
static int __validate_subtree(struct rb_node *node)
{
    if (!node)
        return 1;

    // 性质2: 红色节点的子节点必须为黑
    if (rb_is_red(node)) {
        if (node->rb_left)
            assert(rb_is_black(node->rb_left), __FILE__);
        if (node->rb_right)
            assert(rb_is_black(node->rb_right), __FILE__);
    }

    // 递归验证子树
    int left_black_height = __validate_subtree(node->rb_left);
    int right_black_height = __validate_subtree(node->rb_right);

    // 性质3: 各路径黑节点数相同
    assert(left_black_height == right_black_height, __FILE__);

    // 返回当前子树的黑高度
    return rb_is_black(node) ? left_black_height + 1 : left_black_height;
}

/* 红黑树验证 */
void validate_rbtree(struct rb_root *root)
{
    if (!root->rb_node)
        return;

    // 性质1: 根节点必须为黑
    assert(rb_is_black(root->rb_node), __FILE__);

    // 深度优先遍历验证
    __validate_subtree(root->rb_node);
}
#endif
