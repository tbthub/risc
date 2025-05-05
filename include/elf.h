#ifndef __ELF_H__
#define __ELF_H__
#include "std/stddef.h"

struct file;
// Format of an ELF executable file

#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian
#define RISCV_MACHINE 0xF3

// File header
struct elf64_hdr
{
    uint magic;  // must equal ELF_MAGIC
    uchar elf[12];
    ushort type;
    ushort machine;
    uint version;
    uint64_t entry;
    uint64_t phoff;
    uint64_t shoff;
    uint flags;
    ushort ehsize;
    ushort phentsize;
    ushort phnum;
    ushort shentsize;
    ushort shnum;
    ushort shstrndx;
};

// Program section header
struct proghdr
{
    uint32_t type;
    flags_t flags;
    uint64_t off;  // 文件内偏移地址
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t filesz;
    uint64_t memsz;
    uint64_t align;
};

// Values for Proghdr type
#define ELF_PROG_LOAD 1

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC 1
#define ELF_PROG_FLAG_WRITE 2
#define ELF_PROG_FLAG_READ 4

// RISC-V ELF 相关结构
typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16 Elf64_Half;
typedef uint32_t Elf64_Word;
typedef int32 Elf64_Sword;
typedef uint64_t Elf64_Xword;
typedef int64 Elf64_Sxword;

#define PT_LOAD 1
#define SHT_RELA 4
#define SHN_UNDEF 0

#define ELF64_R_SYM(info) ((info) >> 32)
#define ELF64_R_TYPE(info) ((Elf64_Word)(info)&0xffffffff)
#define ELF64_R_INFO(sym, type) (((uint64_t_t)(sym) << 32) | (uint32_t_t)(type))

// https://github.com/riscv-non-isa/riscv-elf-psabi-doc/releases
#define R_RISCV_NONE 0      /* 无重定位 */
#define R_RISCV_32 1        /* 32 位绝对地址 */
#define R_RISCV_64 2        /* 64 位绝对地址 */
#define R_RISCV_RELATIVE 3  /* 基址相对重定位 */
#define R_RISCV_COPY 4      /* 基址相对重定位 */
#define R_RISCV_JUMP_SLOT 5 /* Indicates the symbol associated with a PLT entry */
#define R_RISCV_BRANCH 16   /* 条件分支（12 位偏移） */
#define R_RISCV_JAL 17      /* 无条件跳转（20 位偏移） */
#define R_RISCV_CALL 18     /* 函数调用（AUIPC + JALR） */
#define R_RISCV_CALL_PLT 19 /* 函数调用（AUIPC + JALR） */

typedef struct
{
    Elf64_Word sh_name;        // 节名称索引
    Elf64_Word sh_type;        // 节类型（如 SHT_RELA）
    Elf64_Xword sh_flags;      // 节标志
    Elf64_Addr sh_addr;        // 虚拟地址
    Elf64_Off sh_offset;       // 文件偏移
    Elf64_Xword sh_size;       // 节大小
    Elf64_Word sh_link;        // 关联节索引
    Elf64_Word sh_info;        // 额外信息
    Elf64_Xword sh_addralign;  // 对齐要求
    Elf64_Xword sh_entsize;    // 条目大小（如重定位项）
} Elf64_Shdr;

typedef struct
{
    Elf64_Addr r_offset;
    Elf64_Xword r_info;
    Elf64_Sxword r_addend;
} Elf64_Rela;

typedef struct
{
    Elf64_Word st_name;  // 符号名索引
    unsigned char st_info;
    unsigned char st_other;
    Elf64_Half st_shndx;
    Elf64_Addr st_value;
    Elf64_Xword st_size;
} Elf64_Sym;

typedef struct ElfParser
{
    struct elf64_hdr ehdr;   // ELF头
    struct proghdr *phdrs;   // 程序表数组
    Elf64_Shdr *shdrs;       // 节头表数组
    char *shstrtab;          // 节名字符串表
    Elf64_Sym *dynsym;       // 动态符号表
    char *dynstr;            // 动态符号字符串表
    Elf64_Rela *rela_dyn;    // 动态重定位表
    uint32_t rela_count;       // 重定位条目数
    uint64_t mod_init_offset;  // init函数偏移
    uint64_t mod_exit_offset;  // exit函数偏移
    struct file *file;       // 关联的文件对象
    uint32_t mem_sz;           // ? 模块的BSS在文件显示合并在数据段，占用空间
} ElfParser;

extern void elf_parser_destroy(ElfParser *parser);
extern int elf_parse_dynamic_sections(ElfParser *parser);
extern int elf_parser_init(ElfParser *parser, const char *path);
extern uint32_t elf_ptload_msize(ElfParser *parser);
extern Elf64_Shdr *elf_find_section(ElfParser *parser, const char *section_name);

#endif
