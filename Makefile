# 变量定义
SRC_DIR := .
BUILD_DIR := $(SRC_DIR)/build
OUT_KERNEL_NAME := Kernel.out

# 相关文件位置
KERN_LD_SCRIPT := $(SRC_DIR)/boot/kernel.ld
USER_LD_SCRIPT := $(SRC_DIR)/user/user.ld
USER_SRC := $(SRC_DIR)/user
EXT_TOOLS := $(SRC_DIR)/tools
TEST_SRC := $(SRC_DIR)/test
USER_ELF_DIR := $(BUILD_DIR)/user/elf
# kernel src
# 查找源文件，排除 EXT_TOOLS  USER_SRC 目录
K_SRCS_C := $(shell find $(SRC_DIR) -type f -name '*.c' -not -path "$(EXT_TOOLS)/*" -not -path "$(USER_SRC)/*" -not -path "$(TEST_SRC)/*")
K_SRCS_S := $(shell find $(SRC_DIR) -type f -name '*.S' -not -path "$(EXT_TOOLS)/*" -not -path "$(USER_SRC)/*" -not -path "$(TEST_SRC)/*")
# 目标文件列表
K_OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(K_SRCS_C))
K_OBJS += $(patsubst $(SRC_DIR)/%.S, $(BUILD_DIR)/%.o, $(K_SRCS_S))

# user src
U_SRCS_C := $(shell find $(USER_SRC) -type f -name '*.c')
U_SRCS_S := $(shell find $(USER_SRC) -type f -name '*.S')
# 目标文件列表
U_OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(U_SRCS_C))
U_OBJS += $(patsubst $(SRC_DIR)/%.S, $(BUILD_DIR)/%.o, $(U_SRCS_S))


# 目录列表
DIRS := $(sort $(dir $(K_OBJS)))
DIRS += $(sort $(dir $(U_OBJS)))
DIRS += $(USER_ELF_DIR)

# 尝试自动推断正确的工具链前缀
ifndef TOOLPREFIX
TOOLPREFIX := $(shell if riscv64-unknown-elf-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-elf-'; \
	elif riscv64-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-linux-gnu-'; \
	elif riscv64-unknown-linux-gnu-objdump -i 2>&1 | grep 'elf64-big' >/dev/null 2>&1; \
	then echo 'riscv64-unknown-linux-gnu-'; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find a riscv64 version of GCC/binutils." 1>&2; \
	echo "*** To turn off this error, run 'make TOOLPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

CC = $(TOOLPREFIX)gcc
GDB = $(TOOLPREFIX)gdb
AS = $(TOOLPREFIX)as
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

CFLAGS = -Wall -O -Werror -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -mcmodel=medany -fno-common -nostdlib
CFLAGS += -ffreestanding -nostdlib -nostdinc -I./include
# CFLAGS += -MMD -MP
LDFLAGS = -z max-page-size=4096  

ASFLAGS =  -I. -I./include

# 请确保这里与 include/param.h NCPU 一致，至少 CPUS >= NCPU
# 少的话会导致我们在添加任务时候是轮转添加的，没有CPU编号，任务无法启动
CPUS := 4

QEMU = qemu-system-riscv64
GDBPORT = $(shell expr `id -u` % 5000 + 25000)
QEMUOPTS = -machine virt -bios none -kernel $(BUILD_DIR)/$(OUT_KERNEL_NAME) -m 128M -smp $(CPUS) -nographic
QEMUOPTS += -serial mon:stdio
QEMUOPTS += -global virtio-mmio.force-legacy=false
# QEMUOPTS += -drive file=virtio_disk.img,if=none,format=raw,id=x0
# QEMUOPTS += -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
# QEMUOPTS += -device pci-bridge,id=virtio-mmio-bus.0,chassis=1

QEMUGDB = $(shell if $(QEMU) -help | grep -q '^-gdb'; \
        then echo "-gdb tcp::$(GDBPORT)"; \
        else echo "-s -p $(GDBPORT)"; fi)

GDBFLAGS = -q -ex "target remote :$(GDBPORT)" -ex "layout split" -ex "set confirm off"

# 默认目标
default: $(BUILD_DIR)/$(OUT_KERNEL_NAME)

# 创建必要目录
$(DIRS):
	@mkdir -p $@


# 编译
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(DIRS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o:  $(SRC_DIR)/%.S | $(DIRS)
	$(CC) $(CFLAGS) -c -o $@ $<

# 链接内核
$(BUILD_DIR)/$(OUT_KERNEL_NAME): $(DIRS)  $(K_OBJS) $(KERN_LD_SCRIPT)
	$(LD) $(LDFLAGS) -T $(KERN_LD_SCRIPT) -o $@ $(K_OBJS) 
	# $(OBJCOPY) -S -O binary $@ $(BUILD_DIR)/$(OUT_KERNEL_NAME)_BIN
	$(OBJDUMP) -j .text -S $@ > $(BUILD_DIR)/kernel.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(BUILD_DIR)/kernel.sym

# 清理
.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)

# 启动内核
qemu: $(BUILD_DIR)/$(OUT_KERNEL_NAME)
	$(QEMU) $(QEMUOPTS)

qemu-gdb: $(BUILD_DIR)/$(OUT_KERNEL_NAME)
	$(QEMU) $(QEMUOPTS) -S $(QEMUGDB)

gdb: $(BUILD_DIR)/$(OUT_KERNEL_NAME)
	$(GDB) $(BUILD_DIR)/$(OUT_KERNEL_NAME) $(GDBFLAGS)

fs: virtio_disk.img
	hexdump -C virtio_disk.img | less

mkfs:
	cd tools && make clean && make

