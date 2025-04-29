// ! 这是一个临时文件，仅仅用于测试模块初始化优先级

// #include "core/module.h"
// #include "std/stdio.h"

// static int test5()
// {
//     printk("5\n");
//     return 0;
// }

// static int test4()
// {
//     printk("4\n");
//     return 0;
// }

// static int test3()
// {
//     printk("3\n");
//     return 0;
// }

// static int test2()
// {
//     printk("2\n");
//     return 0;
// }

// static int test1()
// {
//     printk("1\n");
//     return 0;
// }

// module_init_level(test5,INIT_LEVEL_LAST);
// module_init_level(test2,INIT_LEVEL_DEVICE);
// module_init_level(test3,INIT_LEVEL_LATE);
// module_init_level(test1,INIT_LEVEL_CORE);
// module_init_level(test4,INIT_LEVEL_EARLY);

