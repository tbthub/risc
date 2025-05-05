#ifdef __cplusplus
extern "C" {
#endif

#ifndef _VFS_UTIL_H
#define _VFS_UTIL_H
#include <stdint.h>

extern char *construct_full_path(uint8_t *root_path, uint8_t *work_path, uint16_t root_path_size, uint16_t work_path_size, const char *path);
extern int parse_mount_tag(const char *full_path, char **mount_tag, char **open_path);
#endif

#ifdef __cplusplus
}
#endif
