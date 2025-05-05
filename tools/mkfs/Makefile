CC = gcc
CFLAGS = -lm -I . -I../../include

DISK ?= virtio_disk.img
DISK_SIZE = 500M

TARGET = mkfs
SRC = $(shell find $(.) -type f -name '*.c')


.PHONY: mkfs clean

mkfs:
	@echo "Making filesystem to disk $(DISK)"
	@qemu-img create -f raw  $(OUT)/$(DISK) $(DISK_SIZE)
	@$(CC) $(SRC) -o $(TARGET) $(CFLAGS)
	@./$(TARGET) $(OUT)/$(DISK)


clean:
	rm -rf $(TARGET)
	rm -f $(OUT)/$(DISK)
