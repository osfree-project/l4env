
_load	= 0x1000;	/* kernel load address */

OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)
SEARCH_DIR(/lib); SEARCH_DIR(/usr/lib); SEARCH_DIR(/usr/local/lib);

SECTIONS {

  . = _load + SIZEOF_HEADERS;

  .init : {
    KEEP (*(.init))
  }
  
  .text : {
    *(.text .stub .text.* .gnu.linkonce.t.*)
  } = 0x90

  PROVIDE (_ecode = .);

  .fini : {
    KEEP (*(.fini))
  }

  .rodata : {
    *(.rodata .rodata.* .gnu.linkonce.r.*)
  }

  .preinit_array : {
    PROVIDE (__preinit_array_start = .);
    *(.preinit_array)
    PROVIDE (__preinit_array_end = .);
  }

  .init_array : {
    PROVIDE (__init_array_start = .);
    *(.init_array)
    PROVIDE (__init_array_end = .);
  }

  .fini_array : {
    PROVIDE (__fini_array_start = .);
    *(.fini_array)
    PROVIDE (__fini_array_end = .);
  }

  PROVIDE (_etext = .);

  .data : {
    *(.data .data.* .gnu.linkonce.d.*)
    SORT (CONSTRUCTORS)
  }

  .ctors : {
    KEEP (*crtbegin.o(.ctors))
    KEEP (*(EXCLUDE_FILE (*crtend.o ) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
  }

  .dtors : {
    KEEP (*crtbegin.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o ) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
  }

  .got : {
    *(.got.plt)
    *(.got)
  }

  PROVIDE (_edata = .);

  . = ALIGN(4096);
  .initcall : {
    PROVIDE (_initcall_start = .);
    *(.initcall.*)
    _initkip_start = .;
    KEEP(*(.initkip.version))
    KEEP(*(.initkip.features))
    KEEP(*(.initkip.features.fini))
    _initkip_end = .;
    . = ALIGN(4096);
    PROVIDE (_initcall_end = .);
  }

  . = ALIGN(4096);
  .bss : {			/* Linux 2.2 requires BSS to be last */
    *(.dynbss)
    *(.bss .bss.* .gnu.linkonce.b.*)
    *(COMMON)
  }

  PROVIDE (_end = .);

  /DISCARD/ : {
    *(.note)
    *(.note.ABI-tag)
    *(.comment)
  }
}
