#include "core/work.h"
static char *test_a_str = "work - a\n";
static void test_a()
{
    printk("%s", test_a_str);
}

static void test_b()
{
    printk("work - b\n");
}

void work_queue_test()
{
    work_queue_push(test_a,test_a_str);
    work_queue_push(test_b,NULL);
}