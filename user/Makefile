SRC_DIR := .
BUILD_DIR := $(SRC_DIR)/build
LD_SCRIPT := $(SRC_DIR)/user.ld

# 所有 C 文件路径列表（例如: ./src/init.c ./src/sh.c ...）
SRCS_C := $(shell find $(SRC_DIR) -type f -name '*.c')
# 所有可执行目标列表（例如: build/src/init build/src/sh ...）
TARGETS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%, $(SRCS_C))
DIRS := $(sort $(dir $(TARGETS)))

# 自动检测工具链（保持原样）
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

# 工具链定义
CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)as
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

# 编译选项
CFLAGS = -Wall -O -Werror -fno-omit-frame-pointer -ggdb -gdwarf-2
CFLAGS += -mcmodel=medany -fno-common -nostdlib -ffunction-sections -fdata-sections
CFLAGS += -ffreestanding -nostdlib -nostdinc -I./include -I../include

# 链接选项
LDFLAGS = -z max-page-size=4096 -nostdlib -static

# 确保构建目录存在
$(shell mkdir -p $(DIRS))

# 默认目标：编译所有 C 文件生成独立可执行文件
all: $(TARGETS)

# 清理构建产物
clean:
	@rm -rf $(BUILD_DIR)
	@rm -f usys.S usys.o

#------------------------------------------
# 关键规则：生成系统调用入口代码 usys.o
#------------------------------------------
# 用 Perl 脚本生成 usys.S
$(SRC_DIR)/usys.S: $(SRC_DIR)/usys.pl
	@perl $< > $@

# 编译 usys.S 为 usys.o
$(SRC_DIR)/usys.o: $(SRC_DIR)/usys.S
	@$(CC) $(CFLAGS) -c -o $@ $<

#------------------------------------------
# 关键规则：每个 C 文件独立生成可执行文件
#------------------------------------------
# 规则 1: 将 C 文件编译为 .o 文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@$(CC) $(CFLAGS) -c -o $@ $<

# 规则 2: 将 .o 文件和 usys.o 链接为可执行文件
$(BUILD_DIR)/%: $(BUILD_DIR)/%.o $(SRC_DIR)/usys.o
	@$(LD) $(LDFLAGS) -T $(LD_SCRIPT) -o $@ $^
	@$(OBJDUMP) -d $@ > $@.asm
	@od -An -t x1 -v $@ | awk '{for(i=1;i<=NF;i++) printf "0x%s, ", $$i; print ""}' > $@.bin

# 声明依赖关系：确保 usys.S 和 usys.o 先生成
$(TARGETS): $(SRC_DIR)/usys.o