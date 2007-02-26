ENTRY(_start)

SECTIONS
{
  /* Read-only sections, merged into text segment: */
  .text      :
  {
    _stext = .;
    *(.text .gnu.linkonce.t.*)
  } =0x9090

  _etext = .;
  PROVIDE (etext = .);

  __ex_table :
  {
    *(__ex_table)
  }

  /* Adjust the address for the data segment.  We want to adjust up to
     the same address within the page on the next page up.  */
  /* . = ALIGN(0x1000) + ((ALIGN(8) + 0x1000 - ALIGN(0x1000)) & (0x1000 - 1));
   */
  . = ALIGN(0x1000);
  .data    :
  {
    *(.rodata .gnu.linkonce.r.*)
    *(.rodata.*)
    *(.data .gnu.linkonce.d.*)
    *(.ctors)
    *(.dtors)
    . = ALIGN(4);
    __l4env_initcall_start = .;
    *(.l4env_initcall.init)
    __l4env_initcall_end = .;
  }
  _edata  =  .;
  PROVIDE (edata = .);
  __bss_start = .;
  .bss       :
  {
   *(.bss .gnu.linkonce.b.*)
   *(COMMON)
  }
  _end = . ;
  PROVIDE (end = .);
}
