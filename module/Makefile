TOOLPREFIX := riscv64-linux-gnu-

CC = $(TOOLPREFIX)gcc
OBJDUMP = $(TOOLPREFIX)objdump
READELF = $(TOOLPREFIX)readelf

# 这里是模块源文件的实际路径，会直接收集同一目录下的所有C文件
MOD_SRC ?= .

# 实际路径 tools/sym/liberports.so
LIB_DIR = $(abspath ../tools/sym)
LIBS = -L$(LIB_DIR) -lkexports  # -L指定库路径，-l指定库名（去掉lib前缀和.so后缀）


TARGET = mod
SRC = $(shell find $(MOD_SRC) -type f -name '*.c')
DUMP = $(TARGET).dump

# 编译和链接参数（关键修改）
CFLAGS = -fPIE -fPIC -Wall -nostdinc -O0 -I../include  -fno-plt -Werror 
LDFLAGS =  -nostdlib -nodefaultlibs -Wl,--no-relax -Wl,--unresolved-symbols=ignore-all
LDFLAGS += -Wl,--unresolved-symbols=ignore-all -T mod.ld -Wl,-z,now

.PHONY: all

# 构建目标（修改链接顺序）
all: $(TARGET) $(DUMP)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LIBS)
	@od -An -t x1 -v $@ | awk '{for(i=1;i<=NF;i++) printf "0x%s, ", $$i; print ""}' > $@.bin


$(DUMP):$(TARGET)
	@echo "======= 完整分析报告: $@ =======" > $@
	@echo "\n[1] ELF 文件头信息" >> $@
	$(READELF) -h $< >> $@
	@echo "\n[2] 段头信息" >> $@
	$(READELF) -S $< >> $@
	@echo "\n[3] 符号表" >> $@
	$(READELF) -s $< >> $@
	@echo "\n[4] 模块初始化段反汇编 (.mod_init)" >> $@
	$(OBJDUMP) -d -j .kmod_init $< >> $@  
	@echo "\n[5] 模块退出段反汇编 (.mod_exit)" >> $@   
	$(OBJDUMP) -d -j .kmod_exit $< >> $@          
	@echo "\n[6] 反汇编代码 (.text段)" >> $@
	$(OBJDUMP) -d -j .text $< >> $@
	@echo "\n[7] 动态段信息（如适用）" >> $@         
	$(READELF) -d $< >> $@
	@echo "\n======= 分析结束 =======" >> $@

# 清理（保持不变）
clean:
	rm -f $(TARGET) $(FULL_DUMP) $(TARGET).bin $(DUMP)
