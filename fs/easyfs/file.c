#include "file.h"
#include "easyfs.h"
#include "core/proc.h"
#include "error.h"
#include "fs/fcntl.h"
#include "std/string.h"
#include "mm/mm.h"
#include "mm/slab.h"


static struct file *file_alloc(flags_t flags) {
    struct file *f = kmem_cache_alloc(&file_kmem_cache);
    if (!f)
        return NULL;
    f->f_ip  = NULL;
    f->f_off = 0;
    atomic_set(&f->f_ref, 0);
    mutex_init(&f->f_mutex, "file f_mutex");
    return f;
}

static inline void file_free(struct file *f) {
    assert(f != NULL, "file_free\n");
    kmem_cache_free(f);
}

// static struct file *file_dup(struct file *f) {
//     assert(atomic_read(&f->f_ref) > 0, "file_dup\n");
//     atomic_inc(&f->f_ref);
//     return f;
// }

static struct file *file_open(const char *file_path, flags_t flags) {
    struct easy_m_inode *i = efs_d_namei(file_path);
    if (!i) {
        printk("file_open efs_d_namei\n");
        return NULL;
    }

    struct file *f = file_alloc(flags);
    atomic_set(&f->f_ref, 1);
    f->f_ip  = i;
    f->f_off = 0;
    return f;
}

static void file_close(struct file *f) {
    assert(atomic_read(&f->f_ref) > 0, "file_close");
    // 如果自减后为 0，则可以释放
    if (atomic_dec_and_test(&f->f_ref)) {
        efs_i_put(f->f_ip);
        file_free(f);
    }
    // 有其他线程也在引用，则仅仅减
}

static int file_read(struct file *f, void *vaddr, uint32_t len) {
    int r = 0;

    assert(f->f_ip != NULL, "file_read f->f_ip\n");
    if ((r = efs_i_read(f->f_ip, f->f_off, len, vaddr)) >= 0)
        f->f_off += r;
    else
        panic("file_read r: %d\n", r);
    return r;
}

static int file_write(struct file *f, void *vaddr, uint32_t len) {
    int w = 0;
    assert(f->f_ip != NULL, "file_write f->f_ip\n");
    if ((w = efs_i_write(f->f_ip, f->f_off, len, vaddr)) >= 0)
        f->f_off += w;
    else
        panic("file_write\n");
    return w;
}

static int file_llseek(struct file *f, uint32_t offset, int whence) {
    assert(f->f_ip != NULL, "file_llseek f->f_ip\n");

    int ret = 0;
    uint32_t new_offset;
    switch (whence) {
    case VFS_SEEK_SET: // 从文件开头设置偏移量
        if (offset > efs_i_size(f->f_ip)) {
            printk("file_llseek: offset > f->f_ip->i_di.i_size\n");
            ret = -1;
        } else
            ret = f->f_off = offset;
        break;

    case VFS_SEEK_CUR: // 从当前偏移量设置
        new_offset = f->f_off + offset;
        if (new_offset > efs_i_size(f->f_ip) || new_offset < 0) {
            printk("file_llseek: new_offset out of bounds\n");
            ret = -1;
        } else
            ret = f->f_off = new_offset;
        break;

    case SEEK_END: // 从文件末尾设置偏移量
        new_offset = efs_i_size(f->f_ip) + offset;
        if (new_offset > efs_i_size(f->f_ip) || new_offset < 0) {
            printk("file_llseek: new_offset out of bounds\n");
            ret = -1;
        } else
            ret = f->f_off = new_offset;
        break;

    default:
        printk("file_llseek: invalid whence value\n");
        ret = -1;
        break;
    }

    return ret;
}

int8_t efs_open(vfs_file_context_t *context, uint8_t *path, int flags, int mode) {
    struct file *f = file_open((const char *)path, flags);
    if (!f)
        return -1;
    context->file_ptr = (uint8_t *)f;
    return 0;
}

int8_t efs_close(vfs_file_context_t *context) {
    struct file *f = (struct file *)context->file_ptr;
    assert(f != NULL, "efs_close f is NULL!\n");
    file_close(f);
    return 0;
}

int32_t efs_read(vfs_file_context_t *context, uint8_t *buffer, size_t size) {
    struct file *f = (struct file *)context->file_ptr;
    return (int32_t)file_read(f, (void *)buffer, size);
}

int32_t efs_write(vfs_file_context_t *context, uint8_t *buffer, size_t size) {
    struct file *f = (struct file *)context->file_ptr;
    return (int32_t)file_write(f, (void *)buffer, size);
}

int32_t efs_lseek(vfs_file_context_t *context, int32_t offset, int whence) {
    struct file *f = (struct file *)context->file_ptr;
    return (int32_t)file_llseek(f, offset, whence);
}


// inline void file_lock(struct file *f)
// {
//     mutex_lock(&f->f_mutex);  // 锁定文件操作
// }

// inline void file_unlock(struct file *f)
// {
//     mutex_unlock(&f->f_mutex);  // 解锁文件操作
// }

// // TODO 创建、权限检查
// int64 do_open(const char *path, flags_t flags, int mod)
// {
//     struct file *f = file_open(path, flags);
//     if (!f) {
//         printk("No such entity\n");
//         return -ENOENT;
//     }

//     struct files_struct *files = &myproc()->task->files;
//     spin_lock(&files->file_lock);
//     int fd = get_unused_fd(files);
//     if (fd == -EMFILE) {
//         printk("task open file cnt over\n");
//         spin_unlock(&files->file_lock);
//         file_close(f);
//         return -ENOENT;
//     }

//     fd_install(files, fd, f);
//     spin_unlock(&files->file_lock);

//     return fd;
// }

// int64 do_close(fd_t fd)
// {
//     assert(fd > -1 && fd < NOFILE, "do_close fd");
//     struct files_struct *files = &myproc()->task->files;

//     spin_lock(&files->file_lock);
//     fd_close(files, fd);
//     spin_unlock(&files->file_lock);

//     return 0;
// }

// struct file *fget(struct files_struct *files, int fd)
// {
//     assert(fd > -1 && fd < NOFILE, "fget fd");
//     spin_lock(&files->file_lock);
//     struct file *f = files->fdt[fd];
//     if (!f)
//         panic("fget file NULL\n");
//     file_dup(f);  // 防止使用期间被释放
//     spin_unlock(&files->file_lock);
//     return f;
// }

// struct file *fput(struct files_struct *files, struct file *f)
// {
//     assert(f != NULL, "fput");
//     spin_lock(&files->file_lock);
//     file_close(f);
//     spin_unlock(&files->file_lock);
//     return f;
// }

// int64 do_read(int fd, char *user_buff, int count)
// {
//     if (count == 0)
//         return 0;

//     int r = 0;
//     int total = 0;
//     int bytes_to_read;
//     assert(count > 0, "do_read count");

//     struct files_struct *files = &myproc()->task->files;
//     struct file *f = fget(files, fd);

//     char *kbuf = __alloc_page(0);
//     if (!kbuf) {
//         fput(files, f);
//         return -ENOMEM;  // 分配失败
//     }
//     file_lock(f);
//     while (count > 0) {
//         bytes_to_read = min(count, PGSIZE);
//         r = file_read(f, kbuf, bytes_to_read);
//         if (r < 0) {  // 发生错误
//             total = r;
//             break;
//         }
//         if (r == 0)  // EOF
//             break;

//         memcpy(user_buff, kbuf, r);
//         total += r;
//         user_buff += r;
//         count -= r;
//     }
//     file_unlock(f);
//     __free_page(kbuf);
//     fput(files, f);
//     return total;
// }

// int64 do_write(int fd, char *buff, int count)
// {
//     if (count == 0)
//         return 0;

//     int w = 0;
//     int total = 0;
//     int bytes_to_write;
//     assert(count > 0, "do_write count");

//     struct files_struct *files = &myproc()->task->files;
//     struct file *f = fget(files, fd);

//     char *kbuf = __alloc_page(0);
//     if (!kbuf) {
//         fput(files, f);
//         return -ENOMEM;  // 分配失败
//     }
//     file_lock(f);
//     while (count > 0) {
//         bytes_to_write = min(count, PGSIZE);

//         memcpy(kbuf, buff, bytes_to_write);

//         // 执行文件写入
//         w = file_write(f, kbuf, bytes_to_write);
//         if (w < 0) {  // 发生错误
//             total = w;
//             break;
//         }
//         if (w == 0) {  // 磁盘满等异常
//             break;
//         }

//         // 更新状态
//         total += w;
//         buff += w;
//         count -= w;
//     }
//     file_unlock(f);
//     __free_page(kbuf);
//     fput(files, f);
//     return total;
// }
