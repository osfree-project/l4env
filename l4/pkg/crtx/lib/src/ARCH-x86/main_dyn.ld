/* ld script for binaries linked against libld-l4.s.so */
ENTRY(_start)

SECTIONS
{
  /* No program code/data before _stext/_prog_img_start! */

  /* Read-only sections, merged into text segment. The start address of
   * the text segment is : */
  . = 0x08048000 + SIZEOF_HEADERS;
  
  .interp  : { *(.interp)  }
  .hash    : { *(.hash)    }
  .dynsym  : { *(.dynsym)  }
  .dynstr  : { *(.dynstr)  }
  .rel.dyn : { *(.rel.dyn) }
  .rel.bss : { *(.rel.bss) }
  .rel.plt : { *(.rel.plt) }

  .text :
  {
    _stext = .;
    _prog_img_start = _stext;
    *(.text .text.* .gnu.linkonce.t.*)
    KEEP (*(.text.*personality*))
    KEEP (*(.fini))
    LONG(0xc3)  /* terminate .fini */
    *(.rodata .rodata.* .gnu.linkonce.r.*)
  } = 0x9090

  . = ALIGN(0x1000);
  _etext = .;
  PROVIDE (etext = .); /* don't fail if user defines this symbol */

  /* Linux: exception section for uaccess mechanism */
  __ex_table :
  {
    *(__ex_table)
  }

  .eh_frame_hdr : { *(.eh_frame_hdr) }

  /* ensure that data starts at a new L4 page */
  . = ALIGN(0x1000);
  .data :
  {
    *(.data .data.* .gnu.linkonce.d.*)
    KEEP (*(.gnu.linkonce.d.*personality*))
  }
  /* exception frames for C++ */
  .eh_frame : 
  {
    __eh_frame_start__ = .;
    KEEP (*(.eh_frame)) 
    LONG(0) /* terminate .eh_frame */
  }
  .gcc_except_table : { KEEP(*(.gcc_except_table)) }
  .dynamic :  { *(.dynamic) }

  .ctors :
  {
    KEEP (*(.mark_beg_l4dde_ctors))
    KEEP (*(SORT(.l4dde_ctors.?)))
    KEEP (*(SORT(.l4dde_ctors.??)))
    KEEP (*(SORT(.l4dde_ctors.???)))
    KEEP (*(SORT(.l4dde_ctors.????)))
    KEEP (*(SORT(.l4dde_ctors.?????)))
    KEEP (*(.l4dde_ctors))
    KEEP (*(.mark_end_l4dde_ctors))

    KEEP (*(.mark_beg_ctors))
    KEEP (*(.ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.mark_end_ctors))

    KEEP (*(.mark_beg_c_ctors))
    KEEP (*(SORT(.c_ctors.?)))
    KEEP (*(SORT(.c_ctors.??)))
    KEEP (*(SORT(.c_ctors.???)))
    KEEP (*(SORT(.c_ctors.????)))
    KEEP (*(SORT(.c_ctors.?????)))
    KEEP (*(.c_ctors))
    KEEP (*(.mark_end_c_ctors))
  }
  .dtors :
  {
    KEEP (*(.mark_beg_dtors))
    KEEP (*(.dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.mark_end_dtors))

    KEEP (*(.mark_beg_c_sys_dtors))
    KEEP (*(SORT(.c_dtors.?)))
    KEEP (*(SORT(.c_dtors.??)))
    KEEP (*(.mark_end_c_sys_dtors))

    KEEP (*(.mark_beg_c_dtors))
    KEEP (*(SORT(.c_dtors.???)))
    KEEP (*(SORT(.c_dtors.????)))
    KEEP (*(SORT(.c_dtors.?????)))
    KEEP (*(.c_dtors))
    KEEP (*(.mark_end_c_dtors))
  }

  __alt_instructions = .;
  .altinstructions : { *(.altinstructions) }
  __alt_instructions_end = .;
  .altinstr_replacement : { *(.altinstr_replacement) }

  .got : { *(.got.plt) *(.got) }

  _edata = .;
  PROVIDE (edata = .); /* don't fail if user defines this symbol */

  __bss_start = .;
  .bss :
  {
   *(.dynbss)
   *(.bss .bss.* .gnu.linkonce.b.*)
   *(COMMON)
   _end = .;
   PROVIDE (end = .); /* don't fail if user defines this symbol */
   *(.rospace)
  }

  /* No code/data beyond this symbol in program image */
  _prog_img_end = .;

  /* drop the following sections since we don't need them for DROPS */
  /DISCARD/ : {
    *(.comment)
    *(.note)
    *(.note.*)
  }
}

