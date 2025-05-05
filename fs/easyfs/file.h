#ifndef __FS_H__
#define __FS_H__

#include "core/locks/mutex.h"
#include "lib/atomic.h"
#include "param.h"
#include "fs/fcntl.h"


struct file {
    atomic_t f_ref;
    struct easy_m_inode *f_ip;
    uint32_t f_off;
    mutex_t f_mutex; // 读写互斥操作
};

// extern void file_lock(struct file *f);
// extern void file_unlock(struct file *f);

#endif
