#include "mm/kmalloc.h"
#include "core/module.h"
#include  "std/stdio.h"

void haha()
{
    int a = 0;
}


static void *a;
int my_mod_entry()
{
    a = kmalloc(16, 0);
    printk("hello module! a: %p\n",a);
    haha();
    return 0;
}


void my_mod_exit()
{
    kfree(a);
    printk("byebye module. a: %p\n",a);
}

module_init(my_mod_entry);
module_exit(my_mod_exit);
