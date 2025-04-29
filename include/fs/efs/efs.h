#ifdef __cplusplus
extern "C" {
#endif

#ifndef _EFS_H
#define _EFS_H

#include "std/stdint.h"
#include "std/stddef.h"
#include "vfs/vfs_io.h"


extern void efs_io_table_init(vfs_io_t *table);


#endif
#ifdef __cplusplus
}
#endif
