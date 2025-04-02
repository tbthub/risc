#ifndef __FS_H__
#define __FS_H__
#include "core/locks/mutex.h"
#include "lib/atomic.h"
#include "param.h"

#define FILE_READ (1 << 0)
#define FILE_WRITE (1 << 1)
#define FILE_RDWR (FILE_READ | FILE_WRITE) // 0x03

enum SEEK
{
    SEEK_SET, // 从文件的开头计算偏移量。
    SEEK_CUR, // 从当前偏移量开始计算。
    SEEK_END, // 从文件的末尾开始计算。
};

struct file
{
    atomic_t f_ref;
    flags_t f_flags;
    struct easy_m_inode *f_ip;
    uint32 f_off;

    mutex_t f_mutex;    // 读写互斥操作
};

struct files_struct {
    struct file *fdt[NOFILE];
    spinlock_t file_lock;
};

extern struct file *file_dup(struct file *f);
extern struct file *file_open(const char *file_path, flags_t flags);
extern void file_close(struct file *f);
extern int file_read(struct file *f, void *vaddr, uint32 len);
extern int file_write(struct file *f, void *vaddr, uint32 len);
extern int file_llseek(struct file *f, uint32 offset, int whence);

#endif
