#include "dev/devs.h"

struct dev_lists_struct
{
    dev_t devno; // 负责分配设备号
    spinlock_t lock;
    struct list_head bd_list;
} dev_lists;

void devs_init()
{
    spin_init(&dev_lists.lock, "dev_lists");
    INIT_LIST_HEAD(&dev_lists.bd_list);
    dev_lists.devno = 1;
}

// 申请设备号
dev_t devno_alloc()
{
    spin_lock(&dev_lists.lock);
    uint32 devno = dev_lists.devno++;
    spin_unlock(&dev_lists.lock);
    return devno;
}

void devs_add(struct list_head *node)
{
    spin_lock(&dev_lists.lock);
    list_add_tail(node, &dev_lists.bd_list);
    spin_unlock(&dev_lists.lock);
}

void devs_del(struct list_head *node)
{
    spin_lock(&dev_lists.lock);
    list_del(node);
    spin_unlock(&dev_lists.lock);
}