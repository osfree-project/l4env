/* -*- Makefile -*- */
OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm",
              "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

SECTIONS
{
  /* Read-only sections, merged into text segment. The start address of
   * the text segment is : */
  .text :
  {
    _stext = .;
    *(.text.init) 
    *(.init)
    *(.text .text.* .gnu.linkonce.t.*)
    *(.glue_7t) *(.glue_7)
    KEEP (*(.fini))
    LONG(0xc3)  /* terminate .fini */
    . = ALIGN(0x40);
    *(.rodata .rodata.* .gnu.linkonce.r.* .rodata1)
  }

  _etext = .;
  PROVIDE (etext = .); /* don't fail if user defines this symbol */

  /* ensure that data starts at a new L4 page */
  . = ALIGN(4K);
  .data :
  {
    *(.data .data.* .gnu.linkonce.d.*)
    *(.anno)
  }
  /* exception frames for C++ */
  .eh_frame : 
  { 
    KEEP (*(.eh_frame)) 
    LONG(0) /* terminate .eh_frame */
  }
  .gcc_except_table : { *(.gcc_except_table) }
  .dynamic :  { *(.dynamic) }

  . = ALIGN(4);
  .ctors :
  {
    LONG(0)
    /* traverse this list backwards (see __attribute__((init_priority)) */
    __CTOR_LIST__ = .;
    KEEP (*(.ctors))
    KEEP (*(SORT(.ctors.*)))
    __CTOR_END__ = .;
    /* traverse this list forwards */
    __C_CTOR_LIST__ = .;
    KEEP (*(SORT(.c_ctors.?)))
    KEEP (*(SORT(.c_ctors.??)))
    __C_CTOR_SYS_END__ = .;
    KEEP (*(SORT(.c_ctors.???)))
    KEEP (*(SORT(.c_ctors.????)))
    KEEP (*(SORT(.c_ctors.?????)))
    KEEP (*(.c_ctors))
    __C_CTOR_END__ = .;
  }
  . = ALIGN(4);
  .dtors :
  {
    LONG(0)
    /* traverse this list backwards (see __attribute__((init_priority)) */
    __DTOR_LIST__ = .;
    KEEP (*(.dtors))
    KEEP (*(SORT(.dtors.*)))
    __DTOR_END__ = .;
    /* traverse this list forwards */
    __C_DTOR_LIST__ = .;
    KEEP (*(SORT(.c_dtors.?)))
    KEEP (*(SORT(.c_dtors.??)))
    __C_DTOR_SYS_END__ = .;
    KEEP (*(SORT(.c_dtors.???)))
    KEEP (*(SORT(.c_dtors.????)))
    KEEP (*(SORT(.c_dtors.?????)))
    KEEP (*(.c_dtors))
    __C_DTOR_END__ = .;
  }

  .got : { *(.got.plt) *(.got) }

  _edata  =  .;
  PROVIDE (edata = .); /* don't fail if user defines this symbol */

  __bss_start = .;
  .bss :
  {
   *(.dynbss)
   *(.bss .gnu.linkonce.b.*)
   *(COMMON)
  }
  _end = . ;
  PROVIDE (end = .); /* don't fail if user defines this symbol */

  /* Moved here to ensure that these sections are located _after_ the text
   * section. In the other case we would get program sections with a virtual
   * address of 0 */
  .hash    : { *(.hash)    }
  .dynsym  : { *(.dynsym)  }
  .dynstr  : { *(.dynstr)  }
  .rel.dyn : { *(.rel.dyn) }

  /* drop the following sections since we don't need them for DROPS */
  /DISCARD/ : {
    *(.interp)
    *(.comment)
    *(.note)
  }
}

