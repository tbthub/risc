// #include "lib/rbtree.h"

// #define RBTBLACK	(0)
// #define RBTRED		(1)

// /*
//  * RBTree control structure
//  */
// struct rb_tree
// {
// 	rb_node    *root;			/* root node, or RBTNIL if tree is empty */

// 	/* Remaining fields are constant after rbt_create */

// 	uint16		node_size;		/* actual size of tree nodes */
// 	/* The caller-supplied manipulation functions */
// 	rbt_comparator comparator;
// 	rbt_combiner combiner;
// 	rbt_allocfunc allocfunc;
// 	rbt_freefunc freefunc;
// 	/* Passthrough arg passed to all manipulation functions */
// 	void	   *arg;
// };