CC = gcc
CFLAGS = -lm -I. -I../include -I..
TARGET_DIR = out

# qemu-img create -f raw virtio_disk.img 1G


mkfs: mkfs.c mkdir_out
	qemu-img create -f raw ../virtio_disk.img 500M
	$(CC) mkfs.c -o $(TARGET_DIR)/mkfs $(CFLAGS)
	$(TARGET_DIR)/mkfs ../virtio_disk.img

fs: ../virtio_disk.img
	hexdump -C ../virtio_disk.img | less

clean:
	rm -rf $(TARGET_DIR)
	rm -f ../virtio_disk.img

# 创建目标目录
mkdir_out:
	mkdir -p $(TARGET_DIR)