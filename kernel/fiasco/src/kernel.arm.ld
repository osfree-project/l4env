/* -*- c -*- */

/* this file defines the construction of the kernel's virtual address
   space */

_tcbs_1            = 0xd0000000;	/* assumption: 256MB-aligned */
/*_unused1_1         = _mappings_end_1;	/* assumption: 4MB-aligned */
_unused2_1         = 0xea400000;	/* assumption: 4MB-aligned */
_unused3_1         = 0xea800000;	/* assumption: 4MB-aligned */
_service           = 0xeac00000;	/* assumption: 4MB-aligned */
/* 0xeb000000 - 0xee000000 unused */
_ipc_window0_1     = 0xee000000;	/* assumption: 8MB-aligned */
_ipc_window1_1     = 0xee800000;	/* assumption: 8MB-aligned */
/* 0xef000000 - 0xef800000 unused */
_kstatic1_1        = 0xef800000;	/* assumption: 4MB-aligned */
_iobitmap_1        = 0xefc00000;	/* assumption: 4MB-aligned */
_unused4_io_1      = 0xefc80000;        /* assumption: 2nd level field in 
				           page table for IO bitmap */
_physmem_1         = 0x0000000;	        /* assumption: 256MB-aligned */
_kernel_phys_start = _physmem_1;
_kernel_phys_end   = 0xffffffff;


virt_address       = 0xf0000000;
phys_offset        = virt_address - kernel_load_addr;


OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm",
              "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

SECTIONS {


  /DISCARD/ : {			/* Exit code and data		*/
		*(.exitcall.exit)
	      }
	      
  . = kernel_load_addr;
	      
  .text : {
    bootstrap*(.init)
    bootstrap*(.text .text.* .gnu.linkonce.t.*)
    bootstrap*(.glue_7t) *(.glue_7)
    bootstrap*(.got .got.*)
    bootstrap*(.fini)
    
    bootstrap*(.rodata .rodata.* .gnu.linkonce.r.* .rodata1)

    . = ALIGN(0x40);
    
    bootstrap*(.data .data.* .gnu.linkonce.d.*)
    bootstrap*(.anno)

    bootstrap*(.data) 
    bootstrap*(.gnu.linkonce.d*)
  }

  PROVIDE(bootstrap_bss_start = .);
  .bss_bootstrap : {
    bootstrap*(.bss .bss.* COMMON .gnu.linkonce.b.*)
  }
  PROVIDE(bootstrap_bss_end = .);

  . = ALIGN(4K);
  end_of_loader = .;

  . = end_of_loader - kernel_load_addr + virt_address;

  .kip : AT (end_of_loader) {
    _kernel_image_start = .;
    *(.kernel_info_page)
    _initkip_start = .;
    KEEP(*(.initkip.version))
    KEEP(*(.initkip.features))
    KEEP(*(.initkip.features.fini))
    _initkip_end = .;
    . = ALIGN(4K);
  }

  .text_kernel : AT (ADDR(.text_kernel) - phys_offset) {
    crt0.o(.text)
    *(.init)
    *(.text .text.* .gnu.linkonce.t.*)
    *(.glue_7t) *(.glue_7)
    *(.got .got.*)
    *(.fini)

    *(.text.jdb)
     
    PROVIDE (_ecode = .);

    *(.rodata .rodata.* .gnu.linkonce.r.* .rodata1)

    . = ALIGN(0x40);
    
    PROVIDE (_etext = .);

    *(.data .data.* .gnu.linkonce.d.*)
    *(.anno)

    *(.data) 
    *(.gnu.linkonce.d*)
    *(.anno)
    *(.data.jdb)

    *(.init.data)

/* Constructor and destructor lists, ordered by priority.  The lists
   are traversed from end to start, therefore the *_END__ symbols
   precede the *_LIST__ symbols. */

    __CTOR_END__ = .;
    CONSTRUCTORS
    KEEP(*(.ctors))
    KEEP(*(SORT(.ctors.*)))
    LONG( (__CTOR_LIST__ - __CTOR_END__) / 4 - 1 ) /* number of constructors */
    __CTOR_LIST__ = .;

    __DTOR_END__ = .;    
    KEEP( *(SORT(.dtors.*)))
    KEEP( *(.dtors))
    LONG( (__DTOR_LIST__ - __DTOR_END__) / 4 - 1) /* number of destructors */
    __DTOR_LIST__ = .;

    PROVIDE(_edata  =  .);

  }

  . = ALIGN(4K);
  .bss : AT (ADDR(.bss) - phys_offset) {
    PROVIDE(__bss_start = .);
    *(.bss .bss.* COMMON .gnu.linkonce.b.*)
    *(.bss.jdb)
    PROVIDE(__bss_end = .);
    . = ALIGN(4K);
  }
  __end_of_the_kernel = .;

  ivt_start = ADDR(.bss) + SIZEOF(.bss) - phys_offset;
  .excp 0xffff0000 : AT (ivt_start) {
    *(.vect .excp.text)
    *(.vect.extra)
  }

  . = __end_of_the_kernel + SIZEOF(.excp);

  PROVIDE( _end = . );
    
  . = ALIGN(4K);
  .initcall : AT(ADDR(.initcall) - phys_offset) {
    _initcall_start = .;
    *(.initcall.*)
    . = ALIGN(4K);
    _initcall_end = .;
  } 

  /DISCARD/ :{
    *(.stab .stabstr .stab.excl .stab.exclstr)
    *(.stab.index .stab.indexstr .comment)
  }

}
