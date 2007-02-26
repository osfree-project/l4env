
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

_tcbs_1			= 0x20000000;	/* assumption: 256MB-aligned */
_mappings_1		= 0x50000000;	/* Multipage Slabs */
_mappings_end_1		= 0x5f000000;	/* XXX old code uses _unused1_1 as end-of-region */
_idt                    = 0x5f001000;
_iobitmap_1		= 0x5fc00000;	/* assumption: 4MB-aligned */
_physmem_1		= 0x60000000;	/* Where the physical memory is mapped */
_linuxstack		= 0xbfff0000;	/* Anything above reserved for Linux stack */
_unused1_1		= 0xc0000000;	/* 4MB-aligned (space index)	*/
_unused2_1		= 0xc0400000;	/* 4MB-aligned (chief number)	*/
_unused3_1		= 0xc0800000;	/* 4MB-aligned (host pid)	*/
_unused4_io_1		= 0xc0c00000;	/* assumption: 2nd level field in page table for IO bitmap */
_syscalls		= 0xeacff000;	/* Absolute Syscall Page */

_trampoline_page	= 0xbfff1000;	/* Virt. addr of tramp page in tasks */

load_address		= 0x1000;	/* Where the kernel code gets loaded */

OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)
SEARCH_DIR(/lib); SEARCH_DIR(/usr/lib); SEARCH_DIR(/usr/local/lib);

SECTIONS {

  . = load_address;

  .init : {
    KEEP (*(.init))
  }
  
  .text : {
    *(.text .stub .text.* .gnu.linkonce.t.*)
  }

  .fini : {
    KEEP (*(.fini))
  }

  .rodata : {
    *(.rodata .rodata.* .gnu.linkonce.r.*)
  }

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

  . = ALIGN(4096);
  .bss : {
    *(.dynbss)
    *(.bss .bss.* .gnu.linkonce.b.*)
    *(COMMON)
  }

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
