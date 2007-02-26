/* -*- c -*- */

SECTIONS
{
  .text :
    {
      *(.text .text.*)
    }
  .data :
    {
      *(.data)
    }
  .bss :
    {
      *(.bss)
    }
  .rodata :
    {
      *(.rodata)
    }
  /DISCARD/ :		     /* discard all sections not stripped by `strip' */
    {
      *(*)
    }
}
