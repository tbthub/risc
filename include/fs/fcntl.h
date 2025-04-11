#ifdef __cplusplus
extern "C" {
#endif

#ifndef _FCNTL_H
#define _FCNTL_H

#include "vfs/vfs_io.h"

#define O_RDONLY VFS_O_RDONLY
#define O_WRONLY VFS_O_WRONLY
#define O_RDWR VFS_O_RDWR

#define O_CREAT VFS_O_CREAT
#define O_APPEND VFS_O_APPEND

#define SEEK_SET VFS_SEEK_SET
#define SEEK_CUR VFS_SEEK_CUR
#define SEEK_END VFS_SEEK_END

typedef vfs_off_t off_t;

#endif


#ifdef __cplusplus
}
#endif
