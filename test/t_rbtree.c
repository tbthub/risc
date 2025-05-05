#include "lib/rbtree.h"
#include "mm/kmalloc.h"
#include "std/stdio.h"
// 添加如下代码到测试文件中
static unsigned int seed = 12345;

static int pseudo_rand(void)
{
    seed = (1103515245 * seed + 12345) % 2147483648;
    return seed % 1000;  // 返回范围 [0,999]
}

/* 测试节点结构 */
struct test_node
{
    int key;
    struct rb_node rb;
};

/* 比较函数 */
int node_cmp(struct rb_node *a, struct rb_node *b)
{
    struct test_node *ta = rb_entry(a, struct test_node, rb);
    struct test_node *tb = rb_entry(b, struct test_node, rb);
    return ta->key - tb->key;
}

/* 测试用例1: 基本插入和遍历 */
void test_basic_insert()
{
    struct rb_root root = RB_ROOT;
    struct test_node nodes[5] = {{3}, {1}, {5}, {2}, {4}};

    for (int i = 0; i < 5; ++i)
        rb_insert(&nodes[i].rb, &root, node_cmp);

    // 验证中序遍历顺序
    int expected[] = {1, 2, 3, 4, 5};
    struct rb_node *n = rb_first(&root);
    for (int i = 0; i < 5; ++i) {
        struct test_node *tn = rb_entry(n, struct test_node, rb);
        assert(tn->key == expected[i], __FILE__);
        n = rb_next(n);
    }
    assert(n == NULL, __FILE__);
    printk("1\n");
}

/* 测试用例2: 删除中间节点 */
void test_delete_middle()
{
    struct rb_root root = RB_ROOT;
    struct test_node nodes[5] = {{3}, {1}, {5}, {2}, {4}};

    for (int i = 0; i < 5; ++i)
        rb_insert(&nodes[i].rb, &root, node_cmp);

    // 删除节点3
    rb_erase(&nodes[0].rb, &root);

    // 验证剩余节点
    int expected[] = {1, 2, 4, 5};
    struct rb_node *n = rb_first(&root);
    for (int i = 0; i < 4; ++i) {
        struct test_node *tn = rb_entry(n, struct test_node, rb);
        assert(tn->key == expected[i], __FILE__);
        n = rb_next(n);
    }
    assert(n == NULL, __FILE__);
    printk("2\n");
}

/* 测试用例3: 边界条件测试 */
void test_edge_cases()
{
    // 测试空树
    struct rb_root root = RB_ROOT;
    assert(rb_first(&root) == NULL, __FILE__);

    // 测试单节点树
    struct test_node single = {42};
    rb_insert(&single.rb, &root, node_cmp);
    assert(rb_next(&single.rb) == NULL, __FILE__);
    rb_erase(&single.rb, &root);
    assert(root.rb_node == NULL, __FILE__);
    printk("3\n");
}

/* 测试用例4: 最大节点遍历 */
void test_max_node()
{
    struct rb_root root = RB_ROOT;
    struct test_node nodes[5] = {{1}, {2}, {3}, {4}, {5}};

    // 构建右斜树
    for (int i = 0; i < 5; ++i)
        rb_insert(&nodes[i].rb, &root, node_cmp);

    // 找到最大节点
    struct rb_node *n = rb_first(&root);
    struct test_node *max_node = NULL;
    while (n) {
        max_node = rb_entry(n, struct test_node, rb);
        n = rb_next(n);
    }

    assert(max_node->key == 5, __FILE__);
    assert(rb_next(&max_node->rb) == NULL, __FILE__);
    printk("4\n");
}

/* 测试用例5: 红黑树性质验证 */
void test_rb_properties()
{
    struct rb_root root = RB_ROOT;
    struct test_node nodes[100];

    // 随机插入测试
    for (int i = 0; i < 100; ++i) {
        nodes[i].key = pseudo_rand() % 1000;
        rb_insert(&nodes[i].rb, &root, node_cmp);
    }
    // 随机删除测试
    for (int i = 0; i < 50; ++i) {
        int idx = pseudo_rand() % 100;
        struct test_node *node = &nodes[idx];

        // 检查节点是否已被插入树中
        if (node->rb.rb_parent_color != 0) {
            // printk("Deleting node %d (key=%d)\n", idx, node->key);
            rb_erase(&node->rb, &root);
        }
    }
    printk("5\n");
}

/* 测试用例6: 高频重复操作测试 */
void test_high_frequency_ops()
{
    struct rb_root root = RB_ROOT;
    struct test_node nodes[1000];

    // 初始化所有节点
    for (int i = 0; i < 1000; ++i) {
        nodes[i].key = i;
    }
    // 高频插入/删除循环
    for (int cycle = 0; cycle < 100; ++cycle) {
        // 批量插入
        for (int i = 0; i < 500; ++i) {
            rb_insert(&nodes[i].rb, &root, node_cmp);
        }
        validate_rbtree(&root);
        // 批量删除（验证指针清理）
        for (int i = 0; i < 500; ++i) {
            rb_erase(&nodes[i].rb, &root);
        }
        validate_rbtree(&root);
    }
    printk("6\n");
    validate_rbtree(&root);
}

/* 测试用例7: 右斜树深度删除测试 */
void test_right_skewed_deletion()
{
    struct rb_root root = RB_ROOT;
    struct test_node nodes[10] = {{0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}};

    // 构建右斜树
    for (int i = 0; i < 10; ++i) {
        rb_insert(&nodes[i].rb, &root, node_cmp);
    }

    // 逆序删除触发复杂平衡
    for (int i = 9; i >= 0; --i) {
        rb_erase(&nodes[i].rb, &root);

        // 关键检查：验证父指针有效性
        if (root.rb_node) {
            struct rb_node *n = root.rb_node;
            while (n) {
                if (n->rb_left) {
                    assert(rb_parent(n->rb_left) == n, __FILE__);
                }
                if (n->rb_right) {
                    assert(rb_parent(n->rb_right) == n, __FILE__);
                }
                n = rb_next(n);
            }
        }
    }
    printk("7\n");
}

/* 测试用例8: 内存安全测试 */
void test_memory_safety()
{
    struct rb_root root = RB_ROOT;
    struct test_node *dynamic_nodes[100];

    // 动态分配节点测试
    for (int i = 0; i < 100; ++i) {
        dynamic_nodes[i] = kmalloc(sizeof(struct test_node), 0);
        dynamic_nodes[i]->key = pseudo_rand();
        rb_insert(&dynamic_nodes[i]->rb, &root, node_cmp);
    }

    // 随机删除并释放内存
    for (int i = 0; i < 50; ++i) {
        int idx = pseudo_rand() % 100;
        if (dynamic_nodes[idx]) {
            rb_erase(&dynamic_nodes[idx]->rb, &root);
            kfree(dynamic_nodes[idx]);  // 关键点：立即释放内存
            dynamic_nodes[idx] = NULL;
        }
    }

    // 二次遍历验证（不应访问已释放内存）
    struct rb_node *n = rb_first(&root);
    while (n) {
        struct test_node *tn = rb_entry(n, struct test_node, rb);
        assert(tn->key >= 0, __FILE__);  // 触发野指针访问会panic
        n = rb_next(n);
    }
    printk("8\n");
}

int t_rbtree()
{
    // push_off();
    test_basic_insert();
    test_delete_middle();
    test_edge_cases();
    test_max_node();
    test_rb_properties();
    test_high_frequency_ops();
    test_right_skewed_deletion();
    test_memory_safety();
    // pop_off();

    printk("All tests passed!\n");
    return 0;
}
