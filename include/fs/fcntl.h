#ifndef __FCNTL_H__
#define __FCNTL_H__

#define O_RDONLY    0
#define O_WRONLY    (1 << 0)
#define O_RDWR      (1 << 1)
// #define O_CREAT     (1 << 6)    // 文件不存在则创建
// #define O_EXCL      (1 << 7)    // 与 O_CREAT 配合，文件存在则失败
#define O_TRUNC     (1 << 8)    // 打开时清空文件内容
#define O_APPEND    (1 << 10)   // 追加写入模式



#endif