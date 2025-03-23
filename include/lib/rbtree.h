// #ifndef __RBTREE_H__
// #define __RBTREE_H__
// #include "std/stddef.h"

// typedef struct rb_node
// {
//     char color;
//     struct rb_node *left;
//     struct rb_node *right;
//     struct rb_node *parent;
// } rb_node;

// typedef struct rb_tree rb_tree;


// // arg 传递给所有操作函数的附加参数这可以让调用者向各种操作函数传递额外的上下文信息。
// // 例如，在比较、合并、分配或释放节点时，可能需要一些额外的上下文（如自定义配置、状态等）。

// /* Support functions to be provided by caller */
// typedef int (*rbt_comparator)(const rb_node *a, const rb_node *b, void *arg);
// typedef void (*rbt_combiner)(rb_node *existing, const rb_node *newdata, void *arg);
// typedef rb_node *(*rbt_allocfunc)(void *arg);
// typedef void (*rbt_freefunc)(rb_node *x, void *arg);

// extern rb_tree *rbt_create(uint16 node_size,
//                           rbt_comparator comparator,
//                           rbt_combiner combiner,
//                           rbt_allocfunc allocfunc,
//                           rbt_freefunc freefunc,
//                           void *arg);

// extern rb_node *rbt_find(rb_tree *rbt, const rb_node *data);
// extern rb_node *rbt_find_great(rb_tree *rbt, const rb_node *data, int equal_match);
// extern rb_node *rbt_find_less(rb_tree *rbt, const rb_node *data, int equal_match);
// extern rb_node *rbt_leftmost(rb_tree *rbt);

// extern rb_node *rbt_insert(rb_tree *rbt, const rb_node *data, int *isNew);
// extern void rbt_delete(rb_tree *rbt, rb_node *node);

// #endif