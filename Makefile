# 变量定义
SRC_DIR := $(shell pwd)
BUILD_DIR := $(SRC_DIR)/build
OUT := Kernel
TARGET := $(BUILD_DIR)/$(OUT)

DISK := virtio_disk.img
KERN_LD_SCRIPT := $(SRC_DIR)/boot/kernel.ld

# 相关文件位置
USER_DIR := $(SRC_DIR)/user
TOOLS_DIR := $(SRC_DIR)/tools
MODULE_DIR := $(SRC_DIR)/module


# kernel src
# 查找源文件，排除 TOOLS_DIR  USER_DIR 目录
K_SRCS_C := $(shell find $(SRC_DIR) -type f -name '*.c' -not -path "$(TOOLS_DIR)/*" -not -path "$(USER_DIR)/*" -not -path "$(MODULE_DIR)/*")
K_SRCS_S := $(shell find $(SRC_DIR) -type f -name '*.S' -not -path "$(TOOLS_DIR)/*" -not -path "$(USER_DIR)/*" -not -path "$(MODULE_DIR)/*")
# 目标文件列表
K_OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(K_SRCS_C))
K_OBJS += $(patsubst $(SRC_DIR)/%.S, $(BUILD_DIR)/%.o, $(K_SRCS_S))

# 目录列表
DIRS := $(sort $(dir $(K_OBJS)))

TOOLPREFIX = riscv64-linux-gnu-
CC = $(TOOLPREFIX)gcc
GDB = $(TOOLPREFIX)gdb
AS = $(TOOLPREFIX)as
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump
NM = $(TOOLPREFIX)nm


CFLAGS = -Wall -O -Werror -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -mcmodel=medany -fno-common -nostdlib
CFLAGS += -ffreestanding -nostdlib -nostdinc -I./include
CFLAGS += -MMD -MP  # 启用自动依赖生成
LDFLAGS = -z max-page-size=4096  


# 确保CPUS与param.h中的NCPU一致
CPUS := 4

QEMU = qemu-system-riscv64
GDBPORT = $(shell expr `id -u` % 5000 + 25000)
QEMUOPTS = -machine virt -bios none -kernel $(TARGET) -m 128M -smp $(CPUS) -nographic
QEMUOPTS += -serial mon:stdio
QEMUOPTS += -global virtio-mmio.force-legacy=false
QEMUOPTS += -drive file=virtio_disk.img,if=none,format=raw,id=x0
QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0

QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
        then echo "-gdb tcp::$(GDBPORT)"; \
        else echo "-s -p $(GDBPORT)"; fi)

GDBFLAGS = -q -ex "target remote :$(GDBPORT)" -ex "layout split" -ex "set confirm off"



# 默认目标
default: $(TARGET)

# 创建必要目录
$(DIRS):
	@mkdir -p $@

# 编译规则
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(DIRS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.S | $(DIRS)
	$(CC) $(CFLAGS) -c -o $@ $<

# 链接内核
$(TARGET): $(K_OBJS) $(KERN_LD_SCRIPT)
	$(LD) $(LDFLAGS) -T $(KERN_LD_SCRIPT) -o $@ $(K_OBJS)
	$(OBJDUMP) -j .text -S $@ > $(TARGET).asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(TARGET).sym

# 包含自动生成的依赖文件
DEPS := $(K_OBJS:.o=.d) $(U_OBJS:.o=.d)
-include $(DEPS)

# 清理
.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)

# 启动内核等目标保持不变
qemu: $(TARGET) default
	$(QEMU) $(QEMUOPTS)

qemu-gdb: $(TARGET) default
	$(QEMU) $(QEMUOPTS) -S $(QEMUGDB)

gdb: $(TARGET) default
	$(GDB) $(TARGET) $(GDBFLAGS)

fs: $(DISK)
	hexdump -C $(DISK) | less


mkfs:
	$(MAKE) -C $(TOOLS_DIR) mkfs DISK=$(DISK) OUT=$(shell pwd)

mkfs_clean:
	$(MAKE) -C $(TOOLS_DIR) mkfs_clean DISK=$(DISK) OUT=$(shell pwd)

sym:
	$(MAKE) -C $(TOOLS_DIR) sym KERNEL=$(TARGET)

sym_clean:
	$(MAKE) -C $(TOOLS_DIR) sym_clean


