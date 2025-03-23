// create easy-fs on img
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "../fs/easyfs/easyfs.h"

static void write_block(int fd, uint32_t block_no, const void *data, size_t size)
{
    off_t offset = block_no * BLOCK_SIZE;

    // 清空块（填充零）
    char zero_buffer[BLOCK_SIZE] = {0};
    lseek(fd, offset, SEEK_SET);
    write(fd, zero_buffer, sizeof(zero_buffer)); // 先写入全零的数据来清空块

    // 写入数据
    lseek(fd, offset, SEEK_SET); // 将文件指针移动到块的开始位置
    write(fd, data, size);       // 写入实际数据

    printf("write_block: fd:%d, block_no:%d,\tsize:%zu\n", fd, block_no, size);
}

// 磁盘超级块
static struct easy_d_super_block easy_sb;

// 根目录
static struct
{
    // 128 个占位
    char a[128];
    struct easy_d_inode _inode;
} root_inode;

// 目前这个目录里面啥东西也灭有
// static struct easy_dirent root_dirs[1];

#define MAX(x, y) ((x) > (y) ? (x) : (y))

// inode map 的第 0 位不给使用，标记错误
// block map 要注意把超级块、inode 等信息占用的块标记为已使用
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "请提供文件路径作为参数。\n");
        return 1;
    }

    // 获取传入的文件路径
    char *file_path = argv[1];

    // 打开文件
    int fd = open(file_path, O_RDWR); // 修改为 O_RDWR，以便读取和写入
    if (fd == -1)
    {
        perror("文件打开失败");
        return 1;
    }

    // 获取文件的大小
    off_t disk_size = lseek(fd, 0, SEEK_END) - BLOCK_SIZE; // 使用 lseek 获取文件大小,保留第0个扇区空间
    if (disk_size == -1)
    {
        perror("获取文件大小失败");
        close(fd);
        return 1;
    }

    uint32_t total_blocks = disk_size / BLOCK_SIZE;
    // 计算 inode 数量：取文件数（disk_size / avg_file_size）与磁盘大小的 1% 之间的较大者
    uint32_t inode_count = MAX(disk_size / AVG_FILE_SIZE, total_blocks * 0.01);
    // 计算 inode 位图所占用的块数
    uint32_t inode_map_blocks = (uint32_t)ceil((double)inode_count / 8 / BLOCK_SIZE);
    // 计算 inode 区域所占用的块数
    uint32_t inode_area_blocks = (uint32_t)ceil((double)(inode_count * INODE_SIZE) / BLOCK_SIZE);

    // 数据块数量
    uint32_t data_blocks = total_blocks;
    // 计算数据块位图所占用的块数
    uint32_t data_map_blocks = (uint32_t)ceil((double)data_blocks / 8 / BLOCK_SIZE);

    easy_sb.inode_count = inode_count - 1; // 0 号不用
    easy_sb.inode_free = inode_count - 2;  // 0 和 root
    easy_sb.inode_size = INODE_SIZE;
    easy_sb.inode_map_start = SUPER_BLOCK_LOCATION + 1;
    easy_sb.inode_area_start = easy_sb.inode_map_start + inode_map_blocks;

    easy_sb.block_count = total_blocks;
    easy_sb.block_size = BLOCK_SIZE;
    easy_sb.block_map_start = easy_sb.inode_area_start + inode_area_blocks;
    easy_sb.block_area_start = easy_sb.block_map_start + data_map_blocks;

    easy_sb.block_free = total_blocks - easy_sb.block_area_start - 1; // 除去前面被占用的以及 root 数据块

    strncpy(easy_sb.name, "easy-fs", 16);
    easy_sb.magic = EASYFS_MAGIC;

    // 打印计算结果
    printf("格式化信息:\n");
    printf("  磁盘可用大小: %ld 字节\n", disk_size);
    printf("  总块数: %u\n", total_blocks);
    printf("  inode 数量: %u\n", inode_count);
    printf("  inode 位图占用块数: %u\n", inode_map_blocks);
    printf("  inode 区域占用块数: %u\n", inode_area_blocks);
    printf("  数据块数: %u\n", data_blocks);
    printf("  数据块位图占用块数: %u\n\n", data_map_blocks);

    printf("-- 0 号 inode 不用, 下面是已经分配 root 后的超级块信息 --\n\n");
    // 打印超级块信息
    printf("超级块信息:\n");
    printf("  inode 总数: %u\n", easy_sb.inode_count);
    printf("  inode 空闲数量: %u\n", easy_sb.inode_free);
    printf("  inode 大小: %u 字节\n", easy_sb.inode_size);
    printf("  inode 位图起始块: %u\n", easy_sb.inode_map_start);
    printf("  inode 区域起始块: %u\n", easy_sb.inode_area_start);
    printf("\n");
    printf("  总块数: %u\n", easy_sb.block_count);
    printf("  空闲块数: %u\n", easy_sb.block_free);
    printf("  已经使用块数: %u\n", easy_sb.block_count - easy_sb.block_free);
    printf("  块大小: %u 字节\n", easy_sb.block_size);
    printf("  数据块位图起始块: %u\n", easy_sb.block_map_start);
    printf("  数据块区域起始块: %u\n", easy_sb.block_area_start);
    printf("  文件系统名: %s\n", easy_sb.name);
    printf("  文件系统魔数: %x\n", easy_sb.magic);

    for (int i = 0; i < 128; i++)
    {
        root_inode.a[i] = '\0';
    }

    root_inode._inode.i_no = 1;
    root_inode._inode.i_type = F_DIR;
    root_inode._inode.i_devno = 0;
    root_inode._inode.i_size = 0;
    // root_inode._inode.i_addrs[0] = easy_sb.block_area_start;
    atomic_set(&root_inode._inode.i_nlink, 1);

    printf("\n");
    printf("根目录信息:\n");
    printf("  根目录 inode 编号: %u\n", 1);
    printf("  根目录文件大小: %u\n", root_inode._inode.i_size);
    // printf("  根目录起始数据块: %u\n", root_inode._inode.i_addrs[0]);
    printf("  根目录链接数: %u\n\n", atomic_read(&root_inode._inode.i_nlink));

    int32_t tmp = 0x2;

    // data bitmap 把前面占用的，以及其后紧邻的一个位置给 root
    uint8_t data_map[BLOCK_SIZE / 8] = {0};
    int full_bytes = easy_sb.block_area_start / 8;
    int remaining_bits = easy_sb.block_area_start % 8;

    // 设置前full_bytes个字节为0xFF
    for (int i = 0; i < full_bytes; i++)
        data_map[i] = 0xFF;

    // 设置第full_bytes个字节的前remaining_bits位为1
    if (remaining_bits > 0)
        data_map[full_bytes] = (0xFF << (8 - remaining_bits)) >> (8 - remaining_bits);

    // 设置第0块为未占用
    data_map[0] &= 0xFE;

    // 分配下一个块给root
    int root_block = easy_sb.block_area_start;
    int root_byte = root_block / 8;
    int root_bit = root_block % 8;

    // 检查是否超出data_map的范围
    if (root_byte < (BLOCK_SIZE / 8))
        data_map[root_byte] |= (1 << root_bit);
    else // 处理超出范围的情况，例如记录错误或分配失败
        printf("Error: Root block allocation exceeds data_map size.\n");

    printf("write super block\t\t");
    write_block(fd, SUPER_BLOCK_LOCATION, &easy_sb, sizeof(easy_sb));

    // inode bitmap 将1号位图给 root
    printf("write inode bitmap for root\t");
    write_block(fd, easy_sb.inode_map_start, &tmp, sizeof(tmp));

    // root inode 写入 root 的 d_inode
    printf("write inode for root\t\t");
    write_block(fd, easy_sb.inode_area_start, &root_inode, sizeof(root_inode));

    printf("write block map for root\t");
    write_block(fd, easy_sb.block_map_start, data_map, sizeof(data_map));

    // 关闭文件
    close(fd);

    return 0;
}
