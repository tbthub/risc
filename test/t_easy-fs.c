// #include "core/proc.h"
// #include "mm/mm.h"
// #include "dev/blk/gendisk.h"
// #include "dev/blk/blk_dev.h"
// #include "core/timer.h"
// #include "std/string.h"
// #include "../fs/easyfs/easyfs.h"
// #include "fs/file.h"
// // 必须等到文件系统初始化完成后才可以测试
// static void __easy_fs_test()
// {
//     // thread_timer_sleep(myproc(), 3000);
//     printk("__easy_fs_test\n");

//     // root
//     // struct easy_m_inode *root1 = efs_i_get(1);
//     // struct easy_m_inode *root2 = efs_i_get(1);
//     // assert(root1 == root2 && root1 != NULL, "root1 != root2,root1: %p, root2: %p\n", root1, root2);
//     // efs_i_info(root1);
//     // efs_i_put(root1);
//     // efs_i_put(root2);

//     // new
//     // struct easy_m_inode *new_i = efs_i_new();
//     // struct easy_m_inode *_new_i = efs_i_get(new_i->i_di.i_no);
//     // assert(new_i == _new_i && new_i != NULL, "new_i != _new_i,new_i: %p, _new_i: %p\n", new_i, _new_i);
//     // efs_i_info(new_i);

//     // put & ulink & dup
//     // struct easy_m_inode *i1 = efs_i_get(7);
//     // efs_i_type(i1, F_REG);
//     // efs_i_info(i1);
//     // efs_i_put(i1);
//     // efs_i_unlink(i1);
//     // struct easy_m_inode *i1_ = efs_i_new();
//     // efs_i_info(i1_);
//     // efs_i_dup(i1_);
//     // efs_i_put(i1_);
//     // efs_i_unlink(i1_);

//     // read & write & trunc
//     // struct  easy_m_inode *i2 = efs_i_new();
//     // struct easy_m_inode *i2 = efs_i_new();
//     // char str[] = "hello world\n";
//     // efs_i_write(i2, 0, sizeof(str), str);
//     // struct easy_m_inode *_new = efs_i_new();
//     // int bno;
//     // for (int i = 0; i < 16; i++)
//     // {
//     //     bno = efs_i_bmap(_new, i, 1);
//     //     printk("%d -> bno: %d\n", i, bno);
//     // }
//     // efs_i_info(r);

//     // efs_i_write(i2, 0, sizeof(str), str);
//     // efs_i_trunc(i2);
//     // efs_i_put(i2);
//     // char *s = kmalloc(10, 0);
//     // int rl = efs_i_read(i2, 6, 5, s);
//     // printk("s: %s,read len:%d\n", s, rl);

//     // efs_i_read(r, 2, 3, str2);
//     // printk("%s\n", str2);

//     // struct easy_m_inode *inode1 = efs_i_new();
//     // efs_i_write(inode1, 0, 256 * BLOCK_SIZE, addr);
//     // efs_i_info(inode1);
//     // efs_i_put(inode1);

//     // struct easy_m_inode *inode1 = efs_ialloc(1);
//     // efs_iinfo(inode1);

//     // struct easy_m_inode *inode2 = efs_ialloc(0);
//     // efs_iinfo(inode2);

//     // struct easy_m_inode *inode3 = efs_ialloc(0);
//     // efs_iinfo(inode3);

//     // struct easy_m_inode *inode2_ = efs_ifind(inode2->i_no);
//     // if (inode2_)
//     //     efs_iinfo(inode2_);
//     // else
//     //     printk("aaa\n");

//     struct easy_dentry *root = efs_d_named("/");
//     assert(root == &root_dentry, "root != &root_dentry\n");

//     // // * 1. 创建嵌套目录
//     efs_d_creat(root, "var", F_DIR);
//     efs_d_creat(root, "etc", F_DIR);


//     struct easy_dentry *var_dir = efs_d_named("/var");
//     // struct easy_dentry *etc_dir = efs_d_named("/etc");
//     efs_d_creat(var_dir, "log.txt", F_REG);
//     efs_d_creat(var_dir, "app.txt", F_REG);
//     // struct easy_dentry *etc_config = efs_d_creat(etc_dir, "config.conf", F_REG);
//     efs_d_infos(root);
//     printk("-------------\n");

//     // // * 2. 重命名文件和目录
//     // efs_d_rename(etc_config, "config2.ymal");
//     // assert(etc_config == efs_d_named("/etc/config2.ymal"), "Failed to rename config2\n");
//     // efs_d_rename(var_dir, "new_var");
//     // efs_d_infos(root);
//     // printk("-------------\n");

//     // // * 3.  写入和读取大文件
//     // struct easy_dentry *large_file = efs_d_creat(root, "large_file", F_REG);
//     // assert(large_file != NULL, "Failed to create /large_file\n");

//     // char *large_data = __alloc_pages(0, 10);
//     // memset(large_data, 'A', 1024 * PGSIZE);
//     // efs_d_write(large_file, 0, 1024 * PGSIZE, large_data);
//     // memset(large_data, '\0', 1024 * PGSIZE);

//     // efs_d_read(large_file, 0, 1024 * PGSIZE, large_data);
//     // int count = 0;
//     // while (large_data[count] == 'A')
//     //     count++;
//     // printk("size: %d, count: %d\n", efs_d_fsize(large_file), count);
//     // assert(count == 1024 * PGSIZE, "Error: count == %d", count);
//     // printk("ino: %d, fsize: %d\n", large_file->d_dd.d_ino, large_file->d_inode->i_di.i_size);
//     // efs_d_infos(root);
//     // printk("-------------\n");

//     // // 删除文件
//     // efs_d_unlink(large_file);
//     // printk("ino: %d, fsize: %d\n", large_file->d_dd.d_ino, large_file->d_inode->i_di.i_size);

//     // struct easy_dentry *etc_dir2 = efs_d_named("/etc");
//     // efs_d_unlink(efs_d_named("/etc/config2.ymal"));
//     // efs_d_unlink(etc_dir2);

//     // efs_d_infos(root);
//     // printk("-------------\n");

//     // struct file *f1 = file_open("/var/log.txt", FILE_RDWR);
//     // assert(f1 != NULL, "f1 == NULL");
//     // char w_log[] = "This is log!";
//     // file_write(f1, w_log, sizeof(w_log));

//     // file_llseek(f1, 2, SEEK_SET);

//     // // struct file *f2 = file_open("/var/log.txt", FILE_RDWR);
//     // char r_log[sizeof(w_log)];
//     // int len = file_read(f1, r_log, sizeof(w_log));
//     // printk("%s,len: %d", r_log, len);

//     // 打开文件，设置读写权限
//     struct file *f1 = file_open("/var/log.txt", FILE_RDWR);
//     assert(f1 != NULL, "file_open failed: f1 == NULL");

//     // 写入日志
//     char w_log[] = "This is log!";
//     int write_len = file_write(f1, w_log, sizeof(w_log));
//     assert(write_len == sizeof(w_log), "file_write failed: not all data written");

//     // 移动文件指针
//     int offset = 2;
//     int ret = file_llseek(f1, offset, SEEK_SET);
//     assert(ret == 0, "file_llseek failed: could not seek to offset");

//     // // 读取日志
//     char r_log[sizeof(w_log)];
//     int len = file_read(f1, r_log, sizeof(w_log));
//     assert(len > 0, "file_read failed: no data read");

//     // // 输出读取的内容
//     printk("Read Log: %s, len: %d\n", r_log, len);

//     // // 额外的边界测试：读写大于文件内容的情况
//     char w_log_large[] = "This is a large log that exceeds the previous size.";
//     ret = file_write(f1, w_log_large, sizeof(w_log_large));
//     assert(ret == sizeof(w_log_large), "file_write failed: large log write error");

//     // 检查读写后的文件内容
//     char r_log_large[sizeof(w_log_large)];
//     file_llseek(f1, 0, SEEK_SET); // 从文件开头重新读取
//     len = file_read(f1, r_log_large, sizeof(w_log_large));
//     assert(len == sizeof(w_log_large), "file_read failed: large log read error");
//     printk("Read Large Log: %s, len: %d\n", r_log_large, len);

//     printk("now size of file: %d\n",efs_i_size(f1->f_ip));

//     // 关闭文件操作（如果有此操作）
//     file_close(f1);  // 关闭文件，若有关闭操作

//     printk("File operations test passed successfully.\n");

//     // // 创建新的目录

//     // // 创建子目录

//     // // 创建文件

//     // // 检查目录内容
//     // efs_d_infos(var_dir);
//     // efs_d_infos(etc_dir);

//     // // 测试读取文件内容
//     // char log_content[] = "This is log data in /var/log.txt";
//     // struct easy_dentry *log_file = efs_d_named("/var/log.txt");
//     // efs_d_write(log_file, 0, sizeof(log_content), log_content);

//     // char buffer[100];
//     // efs_d_read(log_file, 0, sizeof(log_content), buffer);
//     // printk("Read from /var/log.txt: %s\n", buffer);

//     // // 删除文件
//     // efs_d_unlink(log_file);
//     // efs_d_infos(var_dir); // Verify that file has been deleted

//     // // 删除目录
//     // efs_d_unlink(var_dir);
//     // efs_d_unlink(etc_dir);
//     // efs_d_rename(etc_dir, "etc_dir2");
//     // efs_d_rename(var_dir, "etc_dir2");
//     // efs_d_infos(root); // Verify that directories have been deleted

//     // struct easy_dentry *rd = efs_d_named("/");
//     // assert(rd == &root_dentry, "rd != &root_dentry\n");

//     // efs_d_info(rd);

//     // efs_d_creat(rd, "bin", F_DIR);
//     // efs_d_creat(rd, "dev", F_DEV);
//     // efs_d_creat(rd, "home", F_DIR);
//     // efs_d_creat(efs_d_named("/home"), "user", F_DIR);
//     // efs_d_creat(efs_d_named("/home/user"), "docs", F_DIR);
//     // efs_d_creat(efs_d_named("/home/user/docs"), "file1.txt", F_REG);
//     // efs_d_creat(efs_d_named("/home/user/docs"), "file2.txt", F_REG);
//     // efs_d_creat(efs_d_named("/home/user/docs"), "file3.txt", F_REG);

//     // struct easy_dentry *docs = efs_d_named("/home/user/docs");
//     // efs_d_lookup(docs);
//     // struct easy_m_inode * docsi = efs_d_namei("/home/user/docs");
//     // printk("aa: %d\n",docsi->i_di.i_size);

//     // efs_d_info(docs);

//     // char file_content[] = "This is the content of file1.txt";
//     // efs_d_write(file1, 0, sizeof(file_content), file_content);

//     // char read_buffer[100];
//     // efs_d_read(file1, 0, sizeof(file_content), read_buffer);

//     // efs_d_infos(rd);
//     // efs_d_unlink(efs_d_named("/home/user/docs"));
//     // efs_d_unlink(efs_d_named("/home/user/docs/file2.txt"));
//     // efs_d_infos(rd);

//     // efs_d_creat(efs_d_named("/bin"), "cat", F_REG);
//     // efs_d_creat(efs_d_named("/bin/cat"), "cat_", F_REG);
//     // efs_d_creat(efs_d_named("/bin/ls"), "ls", F_REG);
//     // efs_d_infos(rd);
//     // efs_d_infos(efs_d_named("/bin"));
//     // char str[] = "This is cat context2";
//     // char str2[] = "This is dev context";

//     // efs_d_creat(efs_d_named("/bin"), "cat", F_REG);
//     // efs_d_write(efs_d_named("/bin/cat"), 0, sizeof(str), str);
//     // efs_d_write_name("/bin/cat", 5, sizeof(str), str);
//     // efs_d_write(efs_d_named("/dev"), 0, sizeof(str2), str2);
//     // efs_d_infos(rd);
//     // efs_d_creat(&root_dentry, "bin", F_DIR);
//     // // printk("a1\n");
//     // efs_d_creat(&root_dentry, "dev", F_DEV);

//     // struct easy_dentry *d = efs_d_namei("/bin");
//     // efs_d_info(d);

//     // efs_d_creat(d, "hello.txt", F_REG);
//     // efs_d_creat(d, "hello1.txt", F_REG);
//     // efs_d_creat(d, "hello3.txt", F_REG);
//     // efs_d_lookup(efs_d_namei("/bin"));

//     // // // printk("a2\n");
//     // efs_d_creat(&root_dentry, "swapfile", F_REG);
//     // efs_d_creat(&root_dentry, "swapfile1", F_REG);
//     // efs_d_creat(&root_dentry, "swapfile2", F_REG);
//     // efs_d_creat(&root_dentry, "swapfile3", F_DIR);
//     // efs_d_creat(&root_dentry, "swapfile4", F_REG);
//     // efs_d_creat(&root_dentry, "swapfile5", F_REG);
//     // efs_d_creat(&root_dentry, "bin", F_DIR);
//     // efs_d_lookup(&root_dentry);
//     // efs_d_creat(&root_dentry, "bin", F_DIR);
//     // efs_d_creat(efs_d_namei("/bin"), "ls", F_REG);
//     // efs_d_creat(efs_d_namei("/bin"), "cat", F_REG);
//     // efs_d_creat(&root_dentry, "dev", F_DEV);

//     // efs_d_infos(efs_d_namei("/bin"),0);
//     // efs_d_info(&root_dentry);
//     // efs_d_info();

//     // efs_d_creat(efs_d_namei("/"), "bin", F_DIR);
//     // struct easy_dentry *bin = efs_d_namei("/bin");
//     // efs_d_creat(bin, "ls", F_DIR);
//     // struct easy_dentry *rd = efs_d_namei("/");
//     // efs_d_infos(rd);
//     // printk("--------\n");
//     // efs_d_lookup(rd);
//     // char str[] = "This is bash context";
//     // // efs_d_write_name("/swapfile", 0, sizeof(str), str);

//     // char *vaddr = __alloc_page(0);
//     // efs_d_read_name("/bin/bash", 0, sizeof(str), vaddr);
//     // printk("%s\n", vaddr);

//     // efs_d_creat(efs_d_namei("/bin"), "bash", F_REG);
//     // efs_d_write_name("/bin/bash", 0, sizeof(str), str);

//     // struct easy_dentry *rd = efs_d_namei("/");
//     // printk("Root directory info before:\n");
//     // efs_d_infos(rd);
//     // printk("--------\n");

//     // // 创建 /bin 目录
//     // printk("Creating /bin directory...\n");
//     // struct easy_dentry *bin_d = efs_d_creat(rd, "bin", F_DIR);
//     // if (!bin_d)
//     // {
//     //     printk("Failed to create /bin!\n");
//     //     return;
//     // }

//     // // 在 /bin 下创建 bash、ls、echo
//     // efs_d_creat(bin_d, "bash", F_REG);
//     // efs_d_creat(bin_d, "ls", F_REG);
//     // efs_d_creat(bin_d, "echo", F_REG);

//     // // 写入 /bin/bash
//     // char str1[] = "This is bash content";
//     // efs_d_write_name("/bin/bash", 0, sizeof(str1), str1);

//     // // 写入 /bin/ls 和 /bin/echo
//     // char str2[] = "LS command";
//     // char str3[] = "Echo command";
//     // efs_d_write_name("/bin/ls", 0, sizeof(str2), str2);
//     // efs_d_write_name("/bin/echo", 0, sizeof(str3), str3);

//     // // 创建 /bin/lib 目录
//     // printk("Creating /bin/lib directory...\n");
//     // struct easy_dentry *lib_d = efs_d_creat(bin_d, "lib", F_DIR);
//     // if (!lib_d)
//     // {
//     //     printk("Failed to create /bin/lib!\n");
//     //     return;
//     // }

//     // // 在 /bin/lib 目录下创建 hello 文件
//     // efs_d_creat(lib_d, "hello", F_REG);
//     // char hello_str[] = "Hello World!";
//     // efs_d_write_name("/bin/lib/hello", 0, sizeof(hello_str), hello_str);

//     // // 读取 /bin/bash
//     // char *vaddr = __alloc_page(0);
//     // efs_d_read_name("/bin/bash", 0, sizeof(str1), vaddr);
//     // printk("Read from /bin/bash: %s\n", vaddr);

//     // // 读取 /bin/ls
//     // efs_d_read_name("/bin/ls", 0, sizeof(str2), vaddr);
//     // printk("Read from /bin/ls: %s\n", vaddr);

//     // // 读取 /bin/echo
//     // efs_d_read_name("/bin/echo", 0, sizeof(str3), vaddr);
//     // printk("Read from /bin/echo: %s\n", vaddr);

//     // // 读取 /bin/lib/hello
//     // efs_d_read_name("/bin/lib/hello", 0, sizeof(hello_str), vaddr);
//     // printk("Read from /bin/lib/hello: %s\n", vaddr);

//     // // 测试 /swapfile
//     // printk("Creating /swapfile...\n");
//     // efs_d_creat(rd, "swapfile", F_REG);
//     // char swap_str[] = "This is swapfile data";
//     // efs_d_write_name("/swapfile", 0, sizeof(swap_str), swap_str);
//     // efs_d_read_name("/swapfile", 0, sizeof(swap_str), vaddr);
//     // printk("Read from /swapfile: %s\n", vaddr);

//     // // 遍历根目录，检查所有文件
//     // printk("Root directory info after:\n");
//     // efs_d_infos(rd);

//     // efs_d_creat(rd,"mnt",F_DIR);
//     // rd = rd->d_parent;
//     // efs_d_creat(rd, "bin", F_DIR);
//     // efs_d_creat(rd, "dev", F_DEV);
//     // efs_d_creat(rd, "swapfile", F_REG);
//     // efs_d_creat(efs_d_namei("/dev"),"tty",F_DEV);
//     // efs_d_info(rd);
//     // efs_d_infos(rd);

//     // printk("---------\n");
//     // efs_d_creat(efs_d_namei("/"), "dev", F_REG);
//     // efs_d_lookup(efs_d_namei("/"));
//     // printk("---------\n");
//     // efs_d_creat(efs_d_namei("/bin"), "hello", F_DIR);
//     // efs_d_infos(efs_d_namei("/"), 3);
// }

// void easy_fs_test()
// {
//     kthread_create(get_init(),__easy_fs_test, NULL, "easy_fs_test",NO_CPU_AFF);
// }
