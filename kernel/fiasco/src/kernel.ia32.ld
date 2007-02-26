/* -*- c -*- */

/* this file defines the construction of the kernel's virtual address
   space */

_tcbs_1			= 0xc0000000;	/* assumption: 256MB-aligned */
_mappings_1		= 0xe0000000;
_mappings_end_1		= 0xea000000;	/* XXX old code uses _unused1_1 as
					   end-of-region */
_unused1_1		= _mappings_end_1;/* assumption: 4MB-aligned */
_unused2_1		= 0xea400000;	/* assumption: 4MB-aligned */
_unused3_1		= 0xea800000;	/* assumption: 4MB-aligned */
_service		= 0xeac00000;	/* assumption: 4MB-aligned */
_idt			= 0xeacfe000;
_syscalls		= 0xeacff000;
_smas_start_1		= 0xeb000000;	/* assumption: 4MB-aligned */
_smas_end_1		= 0xee000000;	/* assumption: 4MB-aligned */
_ipc_window0_1		= 0xee000000;	/* assumption: 8MB-aligned */
_ipc_window1_1		= 0xee800000;	/* assumption: 8MB-aligned */
_smas_version_1		= 0xef000000;   /* assumption: 4MB-aligned */
_smas_area_1		= 0xef400000;	/* assumption: 4MB-aligned */
_kstatic1_1		= 0xef800000;	/* assumption: 4MB-aligned */
_iobitmap_1		= 0xefc00000;	/* assumption: 4MB-aligned */
_unused4_io_1		= 0xefc80000;	/* assumption: 2nd level field in 
					   page table for IO bitmap */
_physmem_1		= 0xf0000000;	/* assumption: 256MB-aligned */
_kernel_phys_start	= _physmem_1;
_kernel_phys_end	= 0xffffffff;

load_address		= 0x1000;

ENTRY(_start)

SECTIONS {

  . = _physmem_1 + load_address;

  .text : {
    crt0.o(.text)
    *(.init)
    *(.text .text.* .gnu.linkonce.t.*)
    *(.fini)

    PROVIDE (_ecode = .);

    *(.rodata .rodata.* .gnu.linkonce.r.*)

    . = ALIGN(0x40);

    PROVIDE (_etext = .);

    *(.data .data.* .gnu.linkonce.d.*)
    *(.anno)

    /* Constructor and destructor lists, ordered by priority.  The lists
       are traversed from end to start, therefore the *_END__ symbols
       precede the *_LIST__ symbols. */

    __CTOR_END__ = .;
    CONSTRUCTORS
    KEEP (*(.ctors))
    KEEP (*(SORT (.ctors.*)))
    LONG ((__CTOR_LIST__ - __CTOR_END__) / 4 - 1) /* number of constructors */
    __CTOR_LIST__ = .;

    __DTOR_END__ = .;    
    KEEP (*(SORT (.dtors.*)))
    KEEP (*(.dtors))
    LONG ((__DTOR_LIST__ - __DTOR_END__) / 4 - 1) /* number of destructors */
    __DTOR_LIST__ = .;

    PROVIDE (_edata = .);
  }

  . = ALIGN(4096);
  .bss : {
    PROVIDE (_bss_start = .);
    *(.bss COMMON)
    PROVIDE (_bss_end = .);
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
