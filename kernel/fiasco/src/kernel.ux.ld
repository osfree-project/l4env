
/* this file defines the construction of the kernel's virtual address
   space */


/*
 * UX Layout
 *
 * 0x00001000-...		Fiasco-UX Text
 *
 * 0x20000000-0x3fffffff	TCB Space
 * 0x50000000-0x5fffffff	Slab Space
 * 0x60000000-0xbffeffff	Physical Memory
 * 0xbfff0000-0xbfffffff	Fiasco-UX Linux Stack
 * 0xc0000000-0xcfffffff	Linux Kernel
 */

_tcbs_1			= 0x20000000;	/* Assumption: 256MB-aligned	*/
_physmem_1		= 0x60000000;	/* Physical memory mapping	*/
_syscalls		= 0xeacff000;	/* Absolute Syscall Page	*/
_utcb_address_page	= 0xbfff0000;	/* UTCB address page		*/
_trampoline_page	= 0xbfff1000;	/* Task Trampoline Page Address */

load_address		= 0x1000;	/* Kernel Load Address		*/

OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)
SEARCH_DIR(/lib); SEARCH_DIR(/usr/lib); SEARCH_DIR(/usr/local/lib);

SECTIONS {

  . = load_address + SIZEOF_HEADERS;

  .init : {
    KEEP (*(.init))
  }
  
  .text : {
    *(.text .stub .text.* .gnu.linkonce.t.*)
  }

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
  .bss : {
    *(.dynbss)
    *(.bss .bss.* .gnu.linkonce.b.*)
    *(COMMON)
  }

  PROVIDE (_end = .);

  . = ALIGN(4096);
  .initcall : {
    PROVIDE (_initcall_start = .);
    *(.initcall.*)
    . = ALIGN(4096);
    PROVIDE (_initcall_end = .);
  }

  /DISCARD/ : {
    *(.note)
    *(.note.ABI-tag)
    *(.comment)
  }
}
