OUTPUT_ARCH( "riscv" )
ENTRY( _entry )

/* 定义程序头表 */
PHDRS
{
    text PT_LOAD FLAGS(0x5); /* 101 */
    kmod_init PT_LOAD FLAGS(0x5);
    kmod_exit PT_LOAD FLAGS(5);
    rodata PT_LOAD FLAGS(0x4); /* 100 */
    data PT_LOAD FLAGS(0x6); /* 110 */
    bss PT_LOAD FLAGS(0x6); /* 110 */
    __ksymtab PT_LOAD FLAGS(0x6); /* 110 */
}


SECTIONS
{
  /*
   * ensure that entry.S / _entry is at 0x80000000,
   * where qemu's -kernel jumps.
   */
  . = 0x80000000;

  .text : {
    KEEP(*(.text._entry))
    *(.text .text.*)
    . = ALIGN(0x1000);
    uservec = .; 
    *(uservec_space)

    . = ALIGN(0x1000);
    ASSERT(. - uservec == 0x1000, "error: uservec larger than one page");

    user_entry = .; 
    *(user_entry_point)   /* 前后各有一个对齐，占一页 */
    . = ALIGN(0x1000);
    ASSERT(. - user_entry == 0x1000, "error: user_entry larger than one page");

    PROVIDE(etext = .);
  } :text

  .kmod_init : {
    PROVIDE(kmod_init_start = .);
    KEEP(*(.kmod_init0 .kmod_init0.*))
    KEEP(*(.kmod_init1 .kmod_init1.*))
    KEEP(*(.kmod_init2 .kmod_init2.*))
    KEEP(*(.kmod_init3 .kmod_init3.*))
    KEEP(*(.kmod_init4 .kmod_init4.*))
    KEEP(*(.kmod_init5 .kmod_init5.*))
    PROVIDE(kmod_init_end = .);
  } :kmod_init

  .kmod_exit : {
    KEEP(*(.kmod_exit5 .kmod_exit5.*))
    KEEP(*(.kmod_exit4 .kmod_exit4.*))
    KEEP(*(.kmod_exit3 .kmod_exit3.*))
    KEEP(*(.kmod_exit2 .kmod_exit2.*))
    KEEP(*(.kmod_exit1 .kmod_exit1.*))
    KEEP(*(.kmod_exit0 .kmod_exit0.*))
  } :kmod_exit

  .rodata : {
    . = ALIGN(16);
    *(.srodata .srodata.*) /* do not need to distinguish this from .rodata */
    . = ALIGN(16);
    *(.rodata .rodata.*)
  } :rodata

  .data : {
    . = ALIGN(16);
    *(.sdata .sdata.*) /* do not need to distinguish this from .data */
    . = ALIGN(16);
    *(.data .data.*)
  } :data

  .bss : {
    . = ALIGN(16);
    *(.sbss .sbss.*) /* do not need to distinguish this from .bss */
    . = ALIGN(16);
    *(.bss .bss.*)
  } :bss

  . = ALIGN(8);
  __ksymtab : {
      __start___ksymtab = .;
      KEEP(*(__ksymtab))
      __stop___ksymtab = .;
    }:__ksymtab

  PROVIDE(end = .);
}
