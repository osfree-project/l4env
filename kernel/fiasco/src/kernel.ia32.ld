/* -*- c -*- */

_boot_sys_start = 0x300000;
_kernel_image	= 0xf0000000;	/* kernel link address */
_load		= 0x1000;	/* skip page 0 to preserve BIOS data */

ENTRY(_boot_start)

SECTIONS {
  . = _boot_sys_start;
  .text_boot : {
    *(.bootstrap.text)
  }
  
  . = ALIGN(0x10);
  .bss_boot : {
    *(.bootstrap.bss)
  }

  . = _kernel_image + _load;

  .text : AT (ADDR(.text) - 0xf0000000) {
    crt0.o(.text)
    *(.init)
    *(.text .text.* .gnu.linkonce.t.*)
    *(.fini)

    PROVIDE (_ecode = .);

    *(.rodata .rodata.* .gnu.linkonce.r.*)

    . = ALIGN(0x40);

    PROVIDE (_etext = .);
  } = 0x90909090

  . = ALIGN(0x10);
  .data : AT (ADDR(.data) - 0xf0000000) {
    *(.data .data.* .gnu.linkonce.d.*)
    *(.anno)

    /* Constructor and destructor lists, ordered by priority.  The lists
       are traversed from end to start, therefore the *_END__ symbols
       precede the *_LIST__ symbols. */

    __CTOR_END__ = .;
    CONSTRUCTORS
    KEEP (*(.ctors))
    KEEP (*(SORT (.ctors.*)))
    LONG ((__CTOR_LIST__ - __CTOR_END__) / 4 - 1) /* number of constructors */
    __CTOR_LIST__ = .;

    __DTOR_END__ = .;    
    KEEP (*(SORT (.dtors.*)))
    KEEP (*(.dtors))
    LONG ((__DTOR_LIST__ - __DTOR_END__) / 4 - 1) /* number of destructors */
    __DTOR_LIST__ = .;

    PROVIDE (_edata = .);
  }

  . = ALIGN(0x10);
  .bss : AT (ADDR(.bss) - 0xf0000000) {
    PROVIDE (_bss_start = .);
    *(.bss .gnu.linkonce.b.* COMMON)
    PROVIDE (_bss_end = .);
  }

  PROVIDE (_end = .);

  . = ALIGN(4096);
  PROVIDE (_initcall_start = .);
  .initcall.text : AT (ADDR(.initcall.text) - 0xf0000000) {
    *(.initcall.text*)
  } = 0x90909090

  .initcall.data : AT (ADDR(.initcall.data) - 0xf0000000) {
    *(.initcall.data*)
    _initkip_start = .;
    KEEP(*(.initkip.version))
    KEEP(*(.initkip.features))
    KEEP(*(.initkip.features.fini))
    _initkip_end = .;
  }
  . = ALIGN(4096);
  PROVIDE (_initcall_end = .);

  /DISCARD/ : {
    *(.note)
    *(.note.ABI-tag)
    *(.comment)
    *(.eh_frame)
  }
}
