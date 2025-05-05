#ifdef __cplusplus
extern "C" {
#endif

#ifndef _VFS_IO_H
#define _VFS_IO_H

#include "vfs_interface.h"
#include <stdint.h>
#include <stddef.h>

struct vfs_io_t;

typedef struct vfs_file_context_t {
    void *file_ptr;
    void *mount_ptr;

    // private
    struct vfs_io_t *op;
    int flag;
    int ref;
    void *lock;
} vfs_file_context_t;

typedef struct vfs_io_t {
    const char *mountTag;
    int8_t (*open)(vfs_file_context_t *, uint8_t *, int, int);
    int32_t (*read)(vfs_file_context_t *, uint8_t *, size_t);
    int32_t (*write)(vfs_file_context_t *, uint8_t *, size_t);
    int8_t (*close)(vfs_file_context_t *);
    int32_t (*lseek)(vfs_file_context_t *, int32_t, int);
    void *(*mount)(void *);
    int8_t (*umount)(void *);

    // private
    void *mount_ptr;
    size_t mountTagSize;

} vfs_io_t;

typedef int vfs_off_t;

#define VFS_O_RDONLY 00
#define VFS_O_WRONLY 01
#define VFS_O_RDWR 02

#define VFS_O_CREAT 0100
#define VFS_O_APPEND 02000

#define VFS_SEEK_SET 0
#define VFS_SEEK_CUR 1
#define VFS_SEEK_END 2

#endif

#ifdef __cplusplus
}
#endif
