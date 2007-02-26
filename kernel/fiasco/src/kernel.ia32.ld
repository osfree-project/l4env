/* -*- c -*- */

/* This defines the construction of the kernel's virtual address space */
/* There are other definitions of addresses defined 
 * as _xxx_addr in src/kern/ia32/kmem-ia32.cpp. Keep this in mind when
 * defining new values.
 */
_tcbs_1			= 0xc0000000;	/* assumption: 256MB-aligned */
_syscalls		= 0xeacff000;
_physmem_1		= 0xf0000000;	/* assumption: 256MB-aligned */
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

  . = ALIGN(0x10);
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
    *(.eh_frame)
  }
}
