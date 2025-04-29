OUTPUT_ARCH("riscv")

PHDRS {
    kmod_init PT_LOAD FLAGS(5);
    kmod_exit PT_LOAD FLAGS(5);
    text PT_LOAD FLAGS(5);
    rodata PT_LOAD FLAGS(4);
    data PT_LOAD FLAGS(6);
    bss PT_LOAD FLAGS(6);
}

SECTIONS {
  . = ALIGN(0x1000);

  .kmod_init : {
    KEEP(*(.kmod_init0 .kmod_init0.*))
    KEEP(*(.kmod_init1 .kmod_init1.*))
    KEEP(*(.kmod_init2 .kmod_init2.*))
    KEEP(*(.kmod_init3 .kmod_init3.*))
    KEEP(*(.kmod_init4 .kmod_init4.*))
    KEEP(*(.kmod_init5 .kmod_init5.*))
  } :kmod_init

  .kmod_exit : {
    KEEP(*(.kmod_exit5 .kmod_exit5.*))
    KEEP(*(.kmod_exit4 .kmod_exit4.*))
    KEEP(*(.kmod_exit3 .kmod_exit3.*))
    KEEP(*(.kmod_exit2 .kmod_exit2.*))
    KEEP(*(.kmod_exit1 .kmod_exit1.*))
    KEEP(*(.kmod_exit0 .kmod_exit0.*))
  } :kmod_exit

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
