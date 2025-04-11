
#include "std/stdint.h"
#include "std/stddef.h"
#include "lib/string.h"
#include "vfs/vfs_interface.h"
#include "vfs/vfs_process.h"

static char *resolve_path(const char *root, const char *path) {
    char *stack[64];
    int depth       = 0;
    const char *p   = path;
    size_t root_len = strlen(root);

    // 手动分割路径组件
    while (*p != '\0') {
        // 跳过所有斜杠
        while (*p == '/')
            p++;
        if (*p == '\0')
            break;

        // 找到当前组件的起始和结束位置
        const char *start = p;
        while (*p != '\0' && *p != '/')
            p++;
        size_t comp_len = p - start;

        // 提取路径组件
        char *component = vfs_malloc(comp_len + 1);
        if (!component) {
            // 内存不足处理
            for (int i = 0; i < depth; i++)
                vfs_free(stack[i]);
            return NULL;
        }
        strncpy(component, start, comp_len);
        component[comp_len] = '\0';

        // 处理特殊组件
        if (strcmp(component, "..") == 0) {
            if (depth > 0) {
                vfs_free(stack[depth - 1]); // 释放前一个组件
                depth--;
            }
        } else if (strcmp(component, ".") != 0) {
            if (depth < 64) {
                stack[depth++] = component;
            } else {
                vfs_free(component); // 栈溢出处理
            }
        } else {
            vfs_free(component); // 忽略"."组件
        }
    }

    // 构建最终路径
    size_t total_len = root_len + 1; // root + /
    for (int i = 0; i < depth; i++) {
        total_len += strlen(stack[i]) + 1; // /component
    }

    char *resolved = vfs_malloc(total_len);
    strcpy(resolved, root);

    // 确保root以/结尾（除非是空root）
    if (root_len > 0 && resolved[root_len - 1] != '/') {
        strcat(resolved, "/");
    }

    // 拼接组件
    for (int i = 0; i < depth; i++) {
        strcat(resolved, stack[i]);
        if (i != depth - 1)
            strcat(resolved, "/");
        vfs_free(stack[i]); // 释放组件内存
    }

    // 处理根目录特殊情况
    if (resolved[strlen(resolved) - 1] == '/' && strlen(resolved) > 1) {
        resolved[strlen(resolved) - 1] = '\0';
    }

    return resolved;
}

char *construct_full_path(uint8_t *root_path, uint8_t *work_path, uint16_t root_path_size, uint16_t work_path_size, const char *path) {
    // 确定基准路径类型
    const uint8_t *base_ptr;
    size_t base_len;
    int need_free_base = 0;
    char *base         = NULL;

    if (path[0] == '/') {
        // 绝对路径：直接使用root_path
        base_ptr = (const uint8_t*) "/";
        base_len = 1;
    } else {
        // 相对路径：需要拼接root_path和work_path
        size_t root_len = 1;
        size_t work_len = work_path_size;

        // 计算合并后的长度（包含可能的斜杠）
        size_t total_base = root_len + work_len + 2;
        base              = vfs_malloc(total_base);
        if (!base)
            return NULL;
        need_free_base = 1;

        base[0]        = '/';
        base[root_len] = '\0';

        // 处理work_path部分
        if (work_len > 0) {
            // 添加中间斜杠（当root不以'/'结尾且work不以'/'开头时）
            if (root_len > 0 && base[root_len - 1] != '/' &&
                work_path[0] != '/') {
                strcat(base, "/");
            }
            strncat(base, (const char *)work_path, work_len);
        }
        base_ptr = (uint8_t *)base;
        base_len = strlen(base);
    }

    // 拼接完整路径
    size_t path_len   = strlen(path);
    size_t total_full = base_len + path_len + 2;

    if (total_full > VFS_MAX_PATH_SIZE) {
        if (need_free_base)
            vfs_free(base);
        return NULL;
    }

    char *full = vfs_malloc(total_full);
    if (!full) {
        if (need_free_base)
            vfs_free(base);
        return NULL;
    }

    // 复制基准路径
    memcpy(full, base_ptr, base_len);
    full[base_len] = '\0';

    // 处理路径连接处的斜杠
    if (base_len > 0) {
        if (full[base_len - 1] != '/' && path[0] != '/') {
            strcat(full, "/"); // 需要添加分隔符
        } else if (full[base_len - 1] == '/' && path[0] == '/') {
            path++; // 跳过多余的斜杠
            path_len--;
        }
    } else {
        // 基准路径为空时强制添加根
        if (path[0] != '/') {
            strcat(full, "/");
            base_len = 1;
        }
    }

    // 追加目标路径
    strncat(full, path, path_len);

    // 清理临时基准路径
    if (need_free_base)
        vfs_free(base);

    // 规范化处理
    char *resolved = resolve_path((const char *)root_path, full);
    vfs_free(full);
    return resolved;
}

int parse_mount_tag(const char *full_path, char **mount_tag, char **open_path) {
    const char *p = full_path;

    // 跳过起始斜杠
    while (*p == '/')
        p++;

    // 查找第一个目录分隔符
    const char *sep = strchr(p, '/');

    // 提取mount_tag
    size_t tag_len = sep - p;
    *mount_tag     = vfs_malloc(tag_len + 1);
    if (!*mount_tag)
        return -1;
    memcpy(*mount_tag, p, tag_len);
    (*mount_tag)[tag_len] = '\0';

    // 处理剩余路径
    size_t remain_len = strlen(sep);
    *open_path        = vfs_malloc(remain_len + 1);
    if (!*open_path) {
        vfs_free(*mount_tag);
        return -1;
    }
    strcpy(*open_path, remain_len > 0 ? sep : "/"); // 保证至少返回根

    return 0;
}
