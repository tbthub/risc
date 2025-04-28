TOOLPREFIX := riscv64-linux-gnu-

# 工具定义（保持不变）
CC = $(TOOLPREFIX)gcc
OBJDUMP = $(TOOLPREFIX)objdump
READELF = $(TOOLPREFIX)readelf

LIB_DIR = $(abspath ../tools)
LIBS = -L$(LIB_DIR) -lkexports  # -L指定库路径，-l指定库名（去掉lib前缀和.so后缀）

# 编译和链接参数（关键修改）
CFLAGS = -fPIE -fPIC -Wall -nostdinc -O0 -I../include  -fno-plt
LDFLAGS =  -nostdlib -nodefaultlibs -Wl,--no-relax -Wl,--unresolved-symbols=ignore-all
LDFLAGS += -Wl,--unresolved-symbols=ignore-all -T mod.ld -Wl,-z,now

# 目标文件（保持不变）
SRC = mod
TARGET = mod
FULL_DUMP = $(TARGET).full_dump

# 构建目标（修改链接顺序）
all: $(TARGET) dump

$(TARGET): $(SRC).c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LIBS)
	@od -An -t x1 -v $@ | awk '{for(i=1;i<=NF;i++) printf "0x%s, ", $$i; print ""}' > $@.bin


dump: $(FULL_DUMP)

$(FULL_DUMP): $(TARGET)
	@echo "======= 完整分析报告: $@ =======" > $@
	@echo "\n[1] ELF 文件头信息" >> $@
	$(READELF) -h $< >> $@
	@echo "\n[2] 段头信息" >> $@
	$(READELF) -S $< >> $@
	@echo "\n[3] 符号表" >> $@
	$(READELF) -s $< >> $@
	@echo "\n[4] 反汇编代码 (.text段)" >> $@
	$(OBJDUMP) -d -j .text $< >> $@
	@echo "\n[5] 模块初始化段反汇编 (.mod_init)" >> $@
	$(OBJDUMP) -d -j .mod_init $< >> $@  
	@echo "\n[6] 模块退出段反汇编 (.mod_exit)" >> $@   
	$(OBJDUMP) -d -j .mod_exit $< >> $@          
	@echo "\n[7] 动态段信息（如适用）" >> $@         
	$(READELF) -d $< >> $@
	@echo "\n======= 分析结束 =======" >> $@

# 清理（保持不变）
clean:
	rm -f $(TARGET) $(FULL_DUMP) $(TARGET).bin
