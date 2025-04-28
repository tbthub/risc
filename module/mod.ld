OUTPUT_ARCH("riscv")

PHDRS {
    text PT_LOAD FLAGS(5);
    mod_init PT_LOAD FLAGS(5);
    mod_exit PT_LOAD FLAGS(5);
    rodata PT_LOAD FLAGS(4);
    data PT_LOAD FLAGS(6);
    bss PT_LOAD FLAGS(6);
}

SECTIONS {
  . = ALIGN(0x1000);
  .text : {
    *(.text .text.*)
  } :text

  .mod_init : {
    KEEP(*(.mod_init .mod_init.*))
  } :mod_init

  .mod_exit : {
    KEEP(*(.mod_exit .mod_exit.*))
  } :mod_exit

  .rodata : {
    *(.rodata .rodata.*)
  } :rodata

  .data : {
    *(.data .data.*)
  } :data

  .bss : {
    . = ALIGN(0x10);
    __bss_start = .;
    *(.sbss .sbss.*)
    *(.bss .bss.*)
    . = ALIGN(0x1000);
    _end = .;
  } :bss
}
