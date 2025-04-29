#include "mm/kmalloc.h"
#include "core/module.h"
#include  "std/stdio.h"
#include "core/proc.h"

void haha()
{
    // int a = 0;
}

extern struct thread_info test_pro;
static void *a;
static int b = 0;
int my_mod_entry()
{
    a = kmalloc(16, 0);
    printk("hello module! a: %p, test.pid: %d\n",a,test_pro.pid);
    haha();
    b++;
    return 0;
}


void my_mod_exit()
{
    kfree(a);
    printk("byebye module. a: %p\n",a);
}

module_init(my_mod_entry);
module_exit(my_mod_exit);
