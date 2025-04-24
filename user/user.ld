OUTPUT_ARCH( "riscv" )

/* 定义程序头表 */
PHDRS
{
    text    PT_LOAD FLAGS(5);  /* R-X，代码段 */
    rodata  PT_LOAD FLAGS(4);  /* R--，只读数据 */
    data    PT_LOAD FLAGS(6);  /* RW-，已初始化数据 */
    bss     PT_LOAD FLAGS(6);  /* RW-，未初始化数据 */
    eh_frame PT_NULL FLAGS(0x4); /* 100 */
}

SECTIONS
{
  . = 0x200001000;
  .text : {
    *(.text .text.*)
  } :text

  . = ALIGN(0x1000);
  .rodata : {
    . = ALIGN(16);
    *(.srodata .srodata.*) /* do not need to distinguish this from .rodata */
    . = ALIGN(16);
    *(.rodata .rodata.*)
  } :rodata

  
  . = ALIGN(0x1000);
  .data : {
    . = ALIGN(16);
    *(.sdata .sdata.*) /* do not need to distinguish this from .data */
    . = ALIGN(16);
    *(.data .data.*)
  } :data

  . = ALIGN(0x1000);
  .bss : {
    . = ALIGN(16);
    *(.sbss .sbss.*) /* do not need to distinguish this from .bss */
    . = ALIGN(16);
    *(.bss .bss.*)
  } :bss

  .eh_frame : {
       *(.eh_frame)
       *(.eh_frame.*)
   } :eh_frame

  PROVIDE(end = .);
}
