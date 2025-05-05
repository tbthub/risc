1. 文件系统必须带标签
2. 根目录只能挂载与卸载，不可修改

请通过c语言完成下列功能
设计一个简单的vfs功能

提供以下结构体

struct vfs_io_t;

typedef struct vfs_process_t {
    uint8_t *root_path;
    uint16_t root_path_size;
    uint8_t *work_path;
    uint16_t work_path_size;
} vfs_process_t;

typedef struct vfs_file_context_t {
    uint8_t *mount_ptr;
    uint8_t *file_ptr; //由驱动初始化
    struct vfs_io_t *op;
} vfs_file_context_t;

typedef struct vfs_io_t {
    uint8_t *mount_ptr;
    int8_t (*open)(vfs_file_context_t *, const char *, int, int);
    int32_t (*read)(vfs_file_context_t *, uint8_t *, size_t);
    int32_t (*write)(vfs_file_context_t *, uint8_t *, size_t);
    int8_t (*close)(vfs_file_context_t *);
    int32_t (*lseek)(vfs_file_context_t *, int32_t , int) ;
    uint8_t* (*mount)();
    int8_t (*umount)(uint8_t *);
} vfs_io_t;

提供以下函数


int vfs_alloc_fd(vfs_process_t *process);
int vfs_set_fd_context(vfs_process_t *process, int fd, uintptr_t ptr, uint8_t flag); // 负值表示失败
void vfs_get_fd_context(vfs_process_t *process, int fd, uintptr_t *ptr, uint8_t *flag);
void vfs_release_fd(vfs_process_t *process, int fd);

void *vfs_malloc(size_t size);
void vfs_free(void *ptr);

void vfs_raise_err(uint16_t error);

全局键值表数据结构
int8_t vfs_data_global_put(uint8_t *key, size_t key_size, void *value, size_t value_size);
void *vfs_data_global_get(uint8_t *key, size_t key_size);
void vfs_data_global_del(uint8_t *key, size_t key_size);
void vfs_data_global_iter(bool (*callback)(uint8_t *, size_t, void *)); 遍历整个表。当callback返回false时，停止遍历。注意，允许在遍历的途中进行删除，且不会对表产生其他影响。


// 读写锁
void *vfs_rwlock_init();
void vfs_wlock_acquire(void *lock);
void vfs_wlock_release(void *lock);
void vfs_rlock_acquire(void *lock);
void vfs_rlock_release(void *lock);
void vfs_lock_deinit(void *lock);


vfs_process_t* vfs_process_read(); //获取当前进程的vfs状态描述符与读锁
void vfs_process_read_done()
vfs_process_t* vfs_process_write(); //获取当前进程的vfs状态描述符与写锁
void vfs_process_read_done(vfs_process_t* newContext)

#define VFS_O_RDONLY 00
#define VFS_O_WRONLY 01
#define VFS_O_RDWR 02

#define VFS_O_CREAT 0100
#define VFS_O_APPEND 02000

要求编写下列函数

int8_t vfs_mount(vfs_io_t *mountIo,
                 const char* mountTag);
检查全局表中是否存在mountTag。若存在，则返回负值
调用mountIo中的mount函数。若返回NULL则返回负值。否则修改mountIo的mount_ptr作为mountd返回值
在全局表中将mountTag与mountIo相映射，若失败，调用mountIo的umount
返回0

int8_t vfs_umount(const char *mountTag);
检查全局表中是否存在mountTag。若不存在，返回负值
调用相应mountIo的umount函数，若返回负值，则返回负值
全局表中删除mountTag


int vfs_open(
             comst char *path,
             int flag,
             int mode);

1. 将root_path + work_path + path 进行拼接为full_path, 特别的，如果path的开头为/, 则跳过拼接work_path
2. 获取full_path的第一个目录，去除/ 后作为mountTag，其余作为openPath。下列示例中ret的第一个为mountTag，第二个为openPath

.e.g
root_path /
work_path 
path /fatfs/test1.txt
ret: (fatfs, /test1.txt)

root_path /
work_path fatfs
path /simfs/test2.txt
ret: (simfs, /test2.txt)

root_path /fatfs
work_path test
path test3.txt
ret: (fatfs, /test/test3.txt)

root_path /
work_path 
path test4.txt
ret: (test4.txt, )

root_path /fatfs/a
work_path b/c
path test4.txt
ret: (fatfs, /a/b/c/test4.txt)

之后，申请fd号，新建并初始化一个vfs_file_context_t
调用mountIo的open函数，其中第二个参数为openPath，若返回负值，则失败并清理
调用vfs_set_fd_context将fd号与该vfs_file_context_t相映射
返回fd号

int8_t vfs_close(int fd);

int vfs_read(
             int fd,
             uint8_t *buf,
             size_t size);

int vfs_write(
              int fd,
              uint8_t *buf,
              size_t size);

int vfs_lseek(int fd, int offset, int whence)

int vfs_dup(int oldfd);

int vfs_dup2(int fd1, int fd2)

int chroot(const char * path);

char *vfs_getcwd(char *buf, size_t size);
int vfs_chdir(const char * path);
