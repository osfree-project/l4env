/* -*- Makefile -*- */
OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm",
              "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

PHDRS
{
  ro PT_LOAD;
  rw PT_LOAD;
}

SECTIONS
{
  /* No program code/data before _stext/_prog_img_start! */

  /* Read-only sections, merged into text segment. The start address of
   * the text segment is : */
  .text :
  {
    _stext = .;
    _prog_img_start = _stext;
    *(.text.init) 
    *(.init)
    *(.text .text.* .gnu.linkonce.t.*)
    KEEP (*(.text.*personality*))
    *(.glue_7t) *(.glue_7)
    KEEP (*(.fini))
    LONG(0xc3)  /* terminate .fini */
    . = ALIGN(0x40);
    *(.rodata .rodata.* .gnu.linkonce.r.* .rodata1)
  } : ro

  _etext = .;
  PROVIDE (etext = .); /* don't fail if user defines this symbol */

  /* ensure that data starts at a new L4 page */
  . = ALIGN(4K);
  .data :
  {
    *(.data .data.* .gnu.linkonce.d.*)
    *(.anno)
  } : rw
  /* exception frames for C++ */
  .eh_frame : 
  {
    __eh_frame_start__ = .;
    KEEP (*(.eh_frame)) 
    LONG(0) /* terminate .eh_frame */
  } : rw
  .gcc_except_table : { KEEP(*(.gcc_except_table)) }
  .dynamic :  { *(.dynamic) }

  . = ALIGN(4);
  .ctors :
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
  } : rw
  . = ALIGN(4);
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
  } : rw

  .got : { *(.got.plt) *(.got) } : rw

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
  } : rw

  /* No code/data beyond this symbol in program image */
  _prog_img_end = .;

  /* Moved here to ensure that these sections are located _after_ the text
   * section. In the other case we would get program sections with a virtual
   * address of 0 */
  .hash    : { *(.hash)    }
  .dynsym  : { *(.dynsym)  }
  .dynstr  : { *(.dynstr)  }
  .rel.dyn : { *(.rel.dyn) }
  .rel.bss : { *(.rel.bss) }
  .rel.plt : { *(.rel.plt) }

  /* drop the following sections since we don't need them for DROPS */
  /DISCARD/ : {
    *(.interp)
    *(.comment)
    *(.note)
    *(.note.*)
  }
}

