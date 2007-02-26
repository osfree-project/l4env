
virt_address = 0xc0080000;
phys_offset  = 0x00000000;

OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm",
              "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

SECTIONS {

  /DISCARD/ : {			/* Exit code and data		*/
    *(.text.exit)
    *(.data.exit)
    *(.exitcall.exit)
  }

  . = virt_address;
  .text : AT (ADDR(.text) + phys_offset) {
    sigma0_crt0.o(.text)
    *(.init)
    *(.text .text.* .gnu.linkonce.t.*)
    *(.glue_7t) *(.glue_7)
    *(.got*)
    *(.fini)

    . = ALIGN(0x40);
    *(.rodata .rodata.* .gnu.linkonce.r.* .rodata1)

    /* Constructor and destructor lists, ordered by priority.  The lists
       are traversed from end to start, therefore the *_END__ symbols
       precede the *_LIST__ symbols. */

    . = ALIGN(4);
    __CTOR_END__ = .;
    CONSTRUCTORS
    KEEP(*(.ctors))
    KEEP(*(SORT(.ctors.*)))
    LONG( (__CTOR_LIST__ - __CTOR_END__) / 4 - 1 ) /* number of constructors */
    __CTOR_LIST__ = .;

    __DTOR_END__ = .;    
    KEEP( *(SORT(.dtors.*)))
    KEEP( *(.dtors))
    LONG( (__DTOR_LIST__ - __DTOR_END__) / 4 - 1) /* number of destructors */
    __DTOR_LIST__ = .;

  }

  .data : AT (ADDR(.data) + phys_offset) {
    . = ALIGN(4);

    *(.data) 
    *(.gnu.linkonce.d*)
    *(.anno)
  }

  . = ALIGN(4K);
  .bss : AT (ADDR(.bss) + phys_offset) {
    PROVIDE( __bss_start = .);
    *(.bss COMMON)
  }

  /DISCARD/ :{
    *(.stab .stabstr .stab.excl .stab.exclstr)
    *(.stab.index .stab.indexstr .comment)
  }


}
