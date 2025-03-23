#include "lib/list.h"
#include "std/stddef.h"

// 新加入的放在链表尾
// 回收的使用统一从头出

// 对于 LRU 操作，如果需要在多线程访问的情况下,必须加自旋锁保护

typedef struct
{
    flags_t flags;
    struct list_head act_list;
    struct list_head ina_list;
} lru2_list_t;

#define LRU2_A (1 << 0) // 0: 在 ina 中，1: 在 act 中

typedef struct
{
    uint32 access;
    struct list_head list;
} lru2_node_t;

inline void lru2_init(lru2_list_t *lru2)
{
    lru2->flags = 0;
    INIT_LIST_HEAD(&lru2->act_list);
    INIT_LIST_HEAD(&lru2->ina_list);
}

inline void lru2_node_init(lru2_node_t *node)
{
    node->access = 0;
    INIT_LIST_HEAD(&node->list);
}

void lru2_up(lru2_node_t *node, lru2_list_t *lru2)
{
    switch (node->access)
    {
    case 2: // 在 act_list 链表中，再次提到 act_list 尾巴
        list_del(&node->list);
        list_add_tail(&node->list, &lru2->act_list);
        break;

    case 1: // 在 ina_list 链表中，提升到 act_list
        node->access++;
        list_del(&node->list);
        list_add_tail(&node->list, &lru2->act_list);
        break;

    case 0: // 如果是第一次插入，添加到 ina_list
        node->access++;
        list_add_tail(&node->list, &lru2->ina_list);
        break;

    default:
        panic("lru2_upgrade, access: %d\n", node->access);
        break;
    }
}

void lru2_down(lru2_node_t *node, lru2_list_t *lru2)
{
    switch (node->access)
    {
    case 2: // 在 act_list 链表中，降到 ina_list
        node->access--;
        list_del(&node->list);
        list_add_tail(&node->list, &lru2->ina_list);
        break;

    case 1: // 在 ina_list 链表中，则直接取下
        node->access--;
        list_del(&node->list);
        break;

    default:
        panic("lru2_downgrade, access: %d\n", node->access);
        break;
    }
}