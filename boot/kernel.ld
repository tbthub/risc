OUTPUT_ARCH( "riscv" )
ENTRY( _entry )

/* 定义程序头表 */
PHDRS
{
    text PT_LOAD FLAGS(0x5); /* 101 */
    rodata PT_LOAD FLAGS(0x4); /* 100 */
    data PT_LOAD FLAGS(0x6); /* 110 */
    bss PT_LOAD FLAGS(0x6); /* 110 */
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
    _uservec = .; 
    *(uservec_space)
    . = ALIGN(0x1000);
    ASSERT(. - _uservec == 0x1000, "error: trampoline larger than one page");
    PROVIDE(etext = .);
  } :text

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

  PROVIDE(end = .);
}
