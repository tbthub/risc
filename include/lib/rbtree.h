#ifndef _RBTREE_H_
#define _RBTREE_H_

#include "std/stddef.h"

/* 红黑树节点颜色 */
#define RBT_RED 0
#define RBT_BLACK 1

/* 红黑树节点结构 */
struct rb_node
{
    unsigned long rb_parent_color;
    struct rb_node *rb_right;
    struct rb_node *rb_left;
};

/* 红黑树根结构 */
struct rb_root
{
    struct rb_node *rb_node;
};

/* 初始化宏 */
#define RB_ROOT                                                                                                                                                                                        \
    (struct rb_root)                                                                                                                                                                                   \
    {                                                                                                                                                                                                  \
        NULL                                                                                                                                                                                           \
    }

/* 容器宏 */
#define rb_entry(ptr, type, member)                                                                                                                                                                    \
    ({                                                                                                                                                                                                 \
        typeof(ptr) ____ptr = (ptr);                                                                                                                                                                   \
        ____ptr ? container_of(____ptr, type, member) : NULL;                                                                                                                                          \
    })

#define rb_is_red(node) (!rb_color(node))
#define rb_is_black(node) rb_color(node)
#define rb_color(node) ((node)->rb_parent_color & 1)

#define rb_parent(r) ((struct rb_node *)((r)->rb_parent_color & ~3))

/* 接口声明 */
extern void rb_insert(struct rb_node *node, struct rb_root *root, int (*cmp)(struct rb_node *, struct rb_node *));
extern void rb_erase(struct rb_node *node, struct rb_root *root);
extern struct rb_node *rb_search(struct rb_root *root, void *key, int (*cmp)(struct rb_node *, void *));
extern struct rb_node *rb_first(struct rb_root *root);
extern struct rb_node *rb_next(struct rb_node *node);

/* 调试接口 */
#define RBT_DEBUG
#ifdef RBT_DEBUG
extern void validate_rbtree(struct rb_root *root);
#else
static inline void validate_rbtree(struct rb_root *root)
{
}
#endif

#endif /* _RBTREE_H_ */
