#ifndef __FS_H__
#define __FS_H__

#include "core/locks/mutex.h"
#include "lib/atomic.h"
#include "param.h"
#include "fs/fcntl.h"



extern struct file *file_dup(struct file *f);
extern struct file *file_open(const char *file_path, flags_t flags);
extern void file_close(struct file *f);
extern int file_read(struct file *f, void *vaddr, uint32 len);
extern int file_read_no_off(struct file *f, uint32 off, void *vaddr, uint32 len);
extern int file_write(struct file *f, void *vaddr, uint32 len);
extern int file_llseek(struct file *f, uint32 offset, int whence);
extern void file_lock(struct file *f);
extern void file_unlock(struct file *f);

#endif
