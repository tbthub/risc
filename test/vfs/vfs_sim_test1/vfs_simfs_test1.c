// #include <stdio.h>
// #include <stdlib.h>
// #include "std/string.h"
// #include <assert.h>

// // 包含被测试的代码
// #include "vfs/vfs_io.h"
// #include "vfs/vfs_interface.h"
// #include "driver/fs/simfs/simfs.h"

// // 测试配置
// #define TEST_FILE_PATH "/test.txt"
// #define TEST_CONTENT "Hello Virtual FS!"
// #define LARGE_FILE_SIZE 2048

// extern size_t total_allocs;
// extern size_t total_frees;
// extern int malloc_should_fail;

// // 简单锁模拟
// void *vfs_lock_init() { return vfs_malloc(sizeof(int)); }
// void vfs_lock_deinit(void *lock) { vfs_free(lock); }
// void vfs_lock_acquire(void *lock) {}
// void vfs_lock_release(void *lock) {}

// void print_test_result(const char *test_name, int passed) {
//     printf("[%s] %s\n", passed ? "PASS" : "FAIL", test_name);
// }

// void reset_test_env() {
//     malloc_should_fail = 0;
//     total_frees        = 0;
//     total_allocs       = 0;
// }

// // 测试用例
// void test_mount_unmount() {
//     reset_test_env();
//     simfs_t *fs = (simfs_t *)simfs_mount();
//     assert(fs != NULL);
//     assert(fs->file_list == NULL);

//     simfs_umount((uint8_t *)fs);
//     printf("Allocs: %zu, Frees: %zu\n", total_allocs, total_frees);
//     assert(total_allocs == total_frees);
//     print_test_result(__func__, 1);
// }

// void test_file_lifecycle() {
//     reset_test_env();
//     vfs_file_context_t ctx;

//     // Mount FS
//     ctx.mount_ptr = (uint8_t *)simfs_mount();
//     assert(ctx.mount_ptr != NULL);
//     // 测试文件创建
//     int8_t ret = simfs_open(&ctx, (uint8_t *)TEST_FILE_PATH, VFS_O_CREAT, 0);
//     assert(ret == 0);
//     assert(ctx.file_ptr != NULL);

//     // 验证文件属性
//     simfs_file_t *file = (simfs_file_t *)ctx.file_ptr;
//     assert(strcmp(file->node->path, TEST_FILE_PATH) == 0);
//     assert(file->node->size == 0);

//     // 测试关闭
//     assert(simfs_close(&ctx) == 0);
//     assert(ctx.file_ptr == NULL);

//     // 卸载验证
//     simfs_umount(ctx.mount_ptr);

//     assert(total_allocs == total_frees);
//     print_test_result(__func__, 1);
// }

// void test_read_write() {
//     reset_test_env();
//     vfs_file_context_t ctx;
//     ctx.mount_ptr = (uint8_t *)simfs_mount();

//     // 创建并写入文件
//     simfs_open(&ctx, (uint8_t *)TEST_FILE_PATH, VFS_O_CREAT, 0);
//     const char *data = TEST_CONTENT;
//     int32_t written  = simfs_write(&ctx, (uint8_t *)data, strlen(data));
//     assert(written == (int32_t)strlen(data));

//     // 重置读取位置
//     simfs_file_t *file = (simfs_file_t *)ctx.file_ptr;
//     file->pos          = 0;

//     // 读取验证
//     uint8_t buffer[64];
//     int32_t read_bytes = simfs_read(&ctx, buffer, sizeof(buffer));
//     assert(read_bytes == written);
//     assert(memcmp(data, buffer, read_bytes) == 0);

//     simfs_close(&ctx);
//     simfs_umount(ctx.mount_ptr);
//     print_test_result(__func__, 1);
// }

// void test_large_file() {
//     reset_test_env();
//     vfs_file_context_t ctx;
//     ctx.mount_ptr = (uint8_t *)simfs_mount();

//     simfs_open(&ctx, (uint8_t *)"/large.bin", VFS_O_CREAT, 0);

//     // 准备测试数据
//     uint8_t *write_data = malloc(LARGE_FILE_SIZE);
//     memset(write_data, 0xAA, LARGE_FILE_SIZE);

//     // 写入测试
//     int32_t written = simfs_write(&ctx, write_data, LARGE_FILE_SIZE);
//     assert(written == LARGE_FILE_SIZE);

//     // 验证容量扩展
//     simfs_file_t *file = (simfs_file_t *)ctx.file_ptr;
//     assert(file->node->capacity >= LARGE_FILE_SIZE);

//     // 读取验证
//     file->pos            = 0;
//     uint8_t *read_buffer = malloc(LARGE_FILE_SIZE);
//     int32_t read_bytes   = simfs_read(&ctx, read_buffer, LARGE_FILE_SIZE);
//     assert(read_bytes == LARGE_FILE_SIZE);
//     assert(memcmp(write_data, read_buffer, LARGE_FILE_SIZE) == 0);

//     free(write_data);
//     free(read_buffer);
//     simfs_close(&ctx);
//     simfs_umount(ctx.mount_ptr);
//     print_test_result(__func__, 1);
// }

// void test_error_handling() {
//     reset_test_env();
//     vfs_file_context_t ctx;

//     // 测试内存分配失败
//     ctx.mount_ptr      = (uint8_t *)simfs_mount();
//     malloc_should_fail = 1;
//     assert(simfs_open(&ctx, (uint8_t *)"/fail.txt", VFS_O_CREAT, 0) == -1);
//     malloc_should_fail = 0;

//     // 清理
//     if (ctx.mount_ptr)
//         simfs_umount(ctx.mount_ptr);
//     print_test_result(__func__, 1);
// }

// int main() {
//     printf("\n=== Starting VFS Test Suite ===\n");

//     test_mount_unmount();
//     test_file_lifecycle();
//     test_read_write();
//     test_large_file();
//     test_error_handling();

//     printf("\n=== All Tests Completed ===\n");
//     return 0;
// }
