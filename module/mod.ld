OUTPUT_ARCH("riscv")

PHDRS {
    mod_init PT_LOAD FLAGS(5);
    mod_exit PT_LOAD FLAGS(5);
    text PT_LOAD FLAGS(5);
    rodata PT_LOAD FLAGS(4);
    data PT_LOAD FLAGS(6);
    bss PT_LOAD FLAGS(6);
}

SECTIONS {
  . = ALIGN(0x1000);

  .mod_init : {
    KEEP(*(.mod_init .mod_init.*))
  } :mod_init

  .mod_exit : {
    KEEP(*(.mod_exit .mod_exit.*))
  } :mod_exit

  .text : {
    *(.text .text.*)
  } :text

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
