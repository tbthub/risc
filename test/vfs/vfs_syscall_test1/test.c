#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "vfs/vfs_process.h"
#include "vfs/vfs_module.h"
#include "vfs/vfs_interface.h"
#include "driver/fs/simfs/simfs.h"

extern int mem_ref;

void check_mem_ref() {
    assert(mem_ref == 0);
}

void test_simple_proc1() {
    int res = fork();

    assert(res >= 0);

    if (res > 0) {
        wait(NULL);
        return;
    }

    char rp[32];
    char wp[32];
    char cp[32];

    vfs_process_init("/", "", 1, 0);
    vfs_chroot("/fatfs/data");
    vfs_chdir("/test1");

    memset(rp, 0, sizeof(rp));
    memset(wp, 0, sizeof(wp));
    memset(cp, 0, sizeof(cp));
    vfs_process_t *proc = vfs_get_process();
    memcpy(rp, proc->root_path, proc->root_path_size);
    memcpy(wp, proc->work_path, proc->work_path_size);
    assert(strcmp(rp, "/fatfs/data") == 0);
    assert(strcmp(wp, "test1") == 0);

    printf("test_simple_proc successful!\n");
    exit(0);
}

void test_simple_vfs() {
    int res = fork();
    assert(res >= 0);
    if (res > 0) {
        wait(NULL);
        return;
    }

    vfs_process_init("/", "", 1, 0);
    vfs_io_t io_table;
    io_table.mountTag = "simfs";
    simfs_io_table_init(&io_table);

    assert(vfs_mount(&io_table) == 0);
    int fd = vfs_open("/simfs/test1.txt", VFS_O_WRONLY | VFS_O_CREAT, 0);

    assert(fd == 0);

    char *test_str = "test1";
    int write_res  = vfs_write(fd, test_str, strlen(test_str));

    assert(write_res == strlen(test_str));
    assert(vfs_close(fd) == 0);

    int fd2 = vfs_open("/simfs/test1.txt", VFS_O_RDONLY, 0);
    assert(fd2 == 0);

    char read_buf[32];
    memset(read_buf, 0, sizeof(read_buf));

    int read_res = vfs_read(fd2, read_buf, sizeof(read_buf));

    assert(read_res == strlen(test_str));
    assert(strcmp(read_buf, test_str) == 0);
    assert(vfs_close(fd2) == 0);
    assert(vfs_umount("simfs") == 0);

    printf("test_simple_vfs successful!\n");
    exit(0);
}

void test_error_handling() {

    int res = fork();
    assert(res >= 0);
    if (res > 0) {
        wait(NULL);
        return;
    }

    vfs_process_init("/", "", 1, 0);

    // 测试重复挂载
    vfs_io_t fs1;
    fs1.mountTag = "test";
    simfs_io_table_init(&fs1);
    assert(vfs_mount(&fs1) == 0);
    assert(vfs_mount(&fs1) < 0); // 重复挂载同tag

    // 测试不存在的卸载
    assert(vfs_umount("invalid_tag") < 0);

    // 测试无效文件操作
    assert(vfs_open("/nonexist/file", VFS_O_RDONLY, 0) < 0);
    assert(vfs_read(999, NULL, 0) < 0); // 无效fd
    assert(vfs_close(999) < 0);

    printf("test_error_handling successful!\n");
    exit(0);
}

void test_lseek_operations() {

    int res = fork();
    assert(res >= 0);
    if (res > 0) {
        wait(NULL);
        return;
    }

    vfs_process_init("/", "", 1, 0);
    vfs_io_t fs;
    fs.mountTag = "simfs";
    simfs_io_table_init(&fs);
    assert(vfs_mount(&fs) == 0);

    // 测试文件定位
    int fd = vfs_open("/simfs/lseek.txt", VFS_O_RDWR | VFS_O_CREAT, 0);
    assert(fd >= 0);

    // 写入测试数据
    char *data = "12345";
    assert(vfs_write(fd, data, 5) == 5);

    // 定位到文件头读取
    assert(vfs_lseek(fd, 0, VFS_SEEK_SET) == 0);
    char buf[6] = {0};
    assert(vfs_read(fd, buf, 5) == 5);
    assert(strcmp(buf, data) == 0);

    // 定位到中间位置
    assert(vfs_lseek(fd, 2, VFS_SEEK_SET) == 2);
    assert(vfs_read(fd, buf, 2) == 2);
    assert(strncmp(buf, "34", 2) == 0);

    // 相对当前位置定位
    assert(vfs_lseek(fd, 1, VFS_SEEK_CUR) == 5);
    assert(vfs_read(fd, buf, 1) == 0); // 已到文件尾

    // 从文件尾回退
    assert(vfs_lseek(fd, -3, VFS_SEEK_END) == 2);
    assert(vfs_read(fd, buf, 2) == 2);
    assert(strncmp(buf, "34", 2) == 0);

    vfs_close(fd);
    vfs_umount("simfs");
    printf("test_lseek_operations successful!\n");
    exit(0);
}

void test_file_descriptor_ops() {
    int res = fork();
    assert(res >= 0);
    if (res > 0) {
        wait(NULL);
        return;
    }

    vfs_process_init("/", "", 1, 0);
    vfs_io_t io_table;
    io_table.mountTag = "simfs";
    simfs_io_table_init(&io_table);
    assert(vfs_mount(&io_table) == 0);

    // 测试dup/dup2
    int fd  = vfs_open("/simfs/dup_test.txt", VFS_O_RDWR | VFS_O_CREAT, 0);
    int fd2 = vfs_dup(fd);
    assert(fd2 == 1);

    int fd3 = vfs_dup2(fd, 5);
    assert(fd3 == 5);

    // 验证副本描述符有效性
    char *str = "dup_test1";
    char buf[100];
    memset(buf, 0, sizeof(buf));
    assert(vfs_write(fd, str, strlen(str)) == strlen(str));
    assert(vfs_lseek(fd, 0, VFS_SEEK_SET) == 0);
    assert(vfs_read(fd, buf, sizeof(buf)) == strlen(str));
    assert(strcmp(buf, str) == 0);
    vfs_close(fd);

    char *str2 = "dup_test2";
    char buf2[100];
    memset(buf2, 0, sizeof(buf2));
    assert(vfs_write(fd2, str2, strlen(str2)) == strlen(str2));
    assert(vfs_lseek(fd2, 0, VFS_SEEK_SET) == 0);
    assert(vfs_read(fd2, buf2, sizeof(buf2)) == strlen(str2));
    assert(strcmp(buf2, str2) == 0);

    vfs_close(fd2);
    vfs_close(fd3);
    vfs_umount("simfs");

    printf("test_file_descriptor_ops successful!\n");
    exit(0);
}

void test_path_operations() {
    int res = fork();
    assert(res >= 0);
    if (res > 0) {
        wait(NULL);
        return;
    }

    vfs_process_init("/base", "work", 5, 4);

    vfs_io_t io_table;
    io_table.mountTag = "base";
    simfs_io_table_init(&io_table);
    vfs_mount(&io_table);

    // 测试路径解析
    int fd = vfs_open("../test/./../data.txt", VFS_O_CREAT | VFS_O_WRONLY, 0);
    assert(fd >= 0);

    vfs_process_t *proc = vfs_get_process();
    char expected_path[256];
    snprintf(expected_path, sizeof(expected_path), "%s/%s", proc->root_path, "base/data.txt");

    // 测试长路径
    char long_path[1024] = {0};
    memset(long_path, 'a', 128);
    assert(vfs_open(long_path, VFS_O_CREAT | VFS_O_WRONLY, 0) >= 0); // 有效长路径
    memset(long_path, 'a', 1023);
    assert(vfs_open(long_path, VFS_O_CREAT | VFS_O_WRONLY, 0) < 0); // 超长路径

    printf("test_path_operations successful!\n");
    exit(0);
}

void test_cross_process_isolation() {
    pid_t pid = fork();
    assert(pid >= 0);

    if (pid > 0) { // Parent
        wait(NULL);
        vfs_process_t *proc = vfs_get_process();
        assert(strcmp(proc->root_path, "/origin") == 0);
    } else { // Child
        vfs_process_init("/modified", "", 1, 0);
        exit(0);
    }

    printf("test_cross_process_isolation successful!\n");
}

int main() {
    test_simple_proc1();
    test_simple_vfs();
    test_error_handling();
    test_lseek_operations();
    test_file_descriptor_ops();
    test_path_operations();
}
