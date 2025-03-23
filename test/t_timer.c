#include "core/work.h"
#include "core/timer.h"
#include "core/proc.h"
static char *test_a_str = "work a --- h:";
static int a = 0;
static int b = 0;
static void test_a()
{
    printk("%d, %s %d\n", ++a, test_a_str, cpuid());
}
static void test_b()
{
    printk("%d, work b --- h: %d\n", ++b, cpuid());
}

static void timer_create_test()
{
    timer_create(test_a, test_a_str, 30, 11, TIMER_NO_BLOCK);
    timer_create(test_b, NULL, 1000, NO_RESTRICT, TIMER_NO_BLOCK);
}

void timer_test()
{
    work_queue_push(timer_create_test, NULL);
}