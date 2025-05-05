PYTHON = python3
CC = riscv64-linux-gnu-gcc 

CFLAGS = -Wl,--allow-shlib-undefine -shared -fPIC -nostdlib

TARGET = libkexports.so
ASM = sym.S
SRC = ksymtab_asm.py


sym:
	@$(PYTHON) $(SRC) $(KERNEL) $(ASM)
	@$(CC) $(ASM) -o $(TARGET) $(CFLAGS)

clean:
	rm -rf $(ASM)
	rm -rf $(TARGET)

.PHONY: sym clean
