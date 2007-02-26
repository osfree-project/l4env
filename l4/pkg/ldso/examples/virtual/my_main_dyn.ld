/* -*- Makefile -*- */
ENTRY(_start)

SECTIONS
{
  /* No program code/data before _stext/_prog_img_start! */

  /* Read-only sections, merged into text segment. The start address of
   * the text segment is : */
  . = 0x01000000;
  
  .interp : AT (ADDR(.interp) - 0x01000000)
  { 
    *(.interp)
  }
  .hash : AT (ADDR(.hash) - 0x01000000)
  {
    *(.hash)
  }
  .dynsym : AT (ADDR(.dynsym) - 0x01000000)
  {
    *(.dynsym)
  }
  .dynstr : AT (ADDR(.dynstr) - 0x01000000)
  {
    *(.dynstr)
  }
  .rel.dyn : AT (ADDR(.rel.dyn) - 0x01000000)
  {
    *(.rel.dyn)
  }
  .rel.bss : AT (ADDR(.rel.bss) - 0x01000000)
  {
    *(.rel.bss)
  }
  .rel.plt : AT (ADDR(.rel.plt) - 0x01000000)
  {
    *(.rel.plt)
  }
  .text : AT (ADDR(.text) - 0x01000000)
  {
    _stext = .;
    _prog_img_start = _stext;
    *(.text .text.* .gnu.linkonce.t.*)
    KEEP (*(.fini))
    LONG(0xc3)  /* terminate .fini */
    *(.rodata .rodata.* .gnu.linkonce.r.*)
  } = 0x9090

  . = ALIGN(0x1000);
  _etext = .;
  PROVIDE (etext = .); /* don't fail if user defines this symbol */

  /* Linux: exception section for uaccess mechanism */
  __ex_table : AT (ADDR(__ex_table) - 0x01000000)
  {
    *(__ex_table)
  }

  .eh_frame_hdr : AT (ADDR(.eh_frame_hdr) - 0x01000000)
  {
    *(.eh_frame_hdr)
  }

  /* ensure that data starts at a new L4 page */
  . = ALIGN(0x1000);
  .data : AT (ADDR(.data) - 0x01000000)
  {
    *(.data .data.* .gnu.linkonce.d.*)
  }
  /* exception frames for C++ */
  .eh_frame : AT (ADDR(.eh_frame) - 0x01000000)
  { 
    KEEP (*(.eh_frame)) 
    LONG(0) /* terminate .eh_frame */
  }
  .gcc_except_table : AT (ADDR(.gcc_except_table) - 0x01000000)
  {
    *(.gcc_except_table)
  }
  .dynamic : AT (ADDR(.dynamic) - 0x01000000)
  {
    *(.dynamic)
  }
  .ctors : AT (ADDR(.ctors) - 0x01000000)
  {
    KEEP (*(.mark_beg_l4dde_ctors))
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
  .dtors : AT (ADDR(.dtors) - 0x01000000)
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
  .altinstructions : AT (ADDR(.altinstructions) - 0x01000000)
  { 
    *(.altinstructions)
  }
  __alt_instructions_end = .;
  .altinstr_replacement : AT (ADDR(.altinstr_replacement) - 0x01000000)
  {
    *(.altinstr_replacement)
  }

  .got : AT (ADDR(.got) - 0x01000000)
  {
    *(.got.plt) *(.got)
  }

  _edata = .;
  PROVIDE (edata = .); /* don't fail if user defines this symbol */

  __bss_start = .;
  .bss : AT (ADDR(.bss) - 0x01000000)
  {
   *(.dynbss)
   *(.bss .gnu.linkonce.b.*)
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

