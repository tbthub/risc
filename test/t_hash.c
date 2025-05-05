#include "lib/hash.h"
#include "mm/kmalloc.h"
#include "lib/list.h"

struct fox
{
    int id;
    hash_node_t h_node;
};

// 初始化哈希表
static struct hash_table fox_hash_table;

void hash_test()
{
    // 初始化哈希表
    hash_init(&fox_hash_table, 7, "hash_test"); // 哈希表大小为 7

    // 向哈希表中插入 40 个元素
    for (int i = 0; i < 400; i++)
    {
        struct fox *f = kmalloc(sizeof(struct fox), 0);
        f->id = i;

        // 添加到哈希表的尾部
        hash_add_tail(&fox_hash_table, i, &f->h_node);
    }

    // 打印哈希表的调试信息
    struct fox *fox_;
    hash_for_each_entry(fox_, &fox_hash_table, 10, h_node)
    {
        printk("%d ", fox_->id);
    }

    // // 测试查找某个元素
    struct fox *found_fox = NULL;
    int key = 0;
    hash_find(found_fox, &fox_hash_table, id, key, h_node);
    if (found_fox == NULL)
    {
        printk("NOT FOUND!\n");
    }
    else
    {
        printk("found_fox id: %d\n", found_fox->id);
    }

    hash_debug(&fox_hash_table);
    hash_del(found_fox, &fox_hash_table, id, key, h_node);
    if (!found_fox)
    {
        printk("NOT FOUND! when del\n");
    }
    else
    {
        printk("del sucessfully\n");
    }
    hash_del(found_fox, &fox_hash_table, id, key, h_node);
    if (!found_fox)
    {
        printk("NOT FOUND! when del\n");
    }
    else
    {
        printk("del sucessfully\n");
    }
    hash_debug(&fox_hash_table);

    hash_free(&fox_hash_table);
}
