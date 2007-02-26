/*
 * Fiasco-UX
 * Emulation Code for some processor internals (registers, instructions)
 */

INTERFACE:

// Someone should really get rid of this
#include <flux/x86/seg.h>		// for pseudo_descriptor
#include <types.h>
#include "linker_syms.h"

class Emulation
{
public:

  enum {
    /*
     * Physical memory layout
     */
    kernel_start_frame	 = 0x1000,	/* Frame 0x0 special-cased by rmgr */
    multiboot_frame	 = 0x1000,	/* Multiboot info + modules */
    trampoline_frame	 = 0x2000,	/* Trampoline Page */
    utcb_address_frame	 = 0x3000,	/* UTCB address page */
    sigstack_start_frame = 0x4000,	/* Kernel Signal Altstack Start */
    sigstack_end_frame   = 0xc000,	/* Kernel Signal Altstack End */
    kernel_end_frame	 = sigstack_end_frame, /* Last Address used by Kernel */
  };

  static unsigned	get_pdir_addr	(void);
  static void		set_pdir_addr	(unsigned addr);
  static unsigned	get_fault_addr	(void);
  static void		set_fault_addr	(unsigned addr);
  static void		lidt		(pseudo_descriptor *desc);
  static Mword		idt_vector	(Mword trap);

  static unsigned short	get_kernel_cs();
  static unsigned short	get_kernel_ss();

  static void modify_ldt (unsigned entry,
                          unsigned long base_addr,
                          unsigned limit);

  static const unsigned trampoline_page  = 
                        reinterpret_cast<unsigned>(&_trampoline_page);
  static const unsigned utcb_address_page  = 
                        reinterpret_cast<unsigned>(&_utcb_address_page);

private:
  static unsigned	page_dir_addr	asm ("PAGE_DIR_ADDR");
  static unsigned	page_fault_addr	asm ("PAGE_FAULT_ADDR");
  static unsigned long	idt_base;
  static unsigned short	idt_limit;
};

IMPLEMENTATION:

#include <asm/unistd.h>

// work around stupid glibc header changes
// this struct must be the same as in asm/ldt.h
struct user_desc {  
        unsigned int  entry_number;
        unsigned long base_addr;   
        unsigned int  limit;
        unsigned int  seg_32bit:1;
        unsigned int  contents:2; 
	unsigned int  read_exec_only:1;
	unsigned int  limit_in_pages:1;
	unsigned int  seg_not_present:1;
	unsigned int  useable:1;
};

unsigned 	Emulation::page_dir_addr;
unsigned 	Emulation::page_fault_addr;
unsigned long	Emulation::idt_base;
unsigned short	Emulation::idt_limit;

/**
 * Return page directory base address (register cr3)
 * @return Page Directory Base Address
 */
IMPLEMENT inline
unsigned
Emulation::get_pdir_addr()
{
  return page_dir_addr;
}
 
/**
 * Set page directory base address (register cr3)
 * @param addr New Page Directory Base Address   
 */
IMPLEMENT inline
void
Emulation::set_pdir_addr (unsigned addr)
{
  page_dir_addr = addr;
}

/**
 * Return page fault address (register cr2)
 * @return Page Fault Address
 */
IMPLEMENT inline
unsigned
Emulation::get_fault_addr()
{
  return page_fault_addr;
}
 
/**
 * Set page fault address (register cr2)
 * @param addr Page Fault Address   
 */
IMPLEMENT inline
void
Emulation::set_fault_addr (unsigned addr)
{
  page_fault_addr = addr;
}

/**
 * Emulate LIDT instruction
 * @param desc IDT pseudo descriptor
 */
IMPLEMENT inline
void
Emulation::lidt (pseudo_descriptor *desc)
{
  idt_base  = desc->linear_base;
  idt_limit = desc->limit;
}

IMPLEMENT
Mword
Emulation::idt_vector (Mword trap)
{
  return *((Mword *) idt_base + (trap << 1))     & 0x0000ffff |
         *((Mword *) idt_base + (trap << 1) + 1) & 0xffff0000;
}

IMPLEMENT inline
unsigned short
Emulation::get_kernel_cs()
{
  unsigned short cs;
  asm volatile ("movw %%cs, %0" : "=g" (cs));
  return cs;
}

IMPLEMENT inline
unsigned short
Emulation::get_kernel_ss()
{
  unsigned short ss;
  asm volatile ("movw %%ss, %0" : "=g" (ss));
  return ss;
}

IMPLEMENT
void
Emulation::modify_ldt (unsigned entry, unsigned long base_addr, unsigned limit)
{
  struct user_desc ldt;

  ldt.entry_number    = entry;
  ldt.base_addr       = base_addr;
  ldt.limit           = limit;
  ldt.seg_32bit       = 1;
  ldt.contents        = 0;
  ldt.read_exec_only  = 0;
  ldt.limit_in_pages  = 0;
  ldt.seg_not_present = 0;
  ldt.useable         = 1;

  asm volatile ("int $0x80" : : "a" (__NR_modify_ldt),
                                "b" (1),              
                                "c" (&ldt),
                                "d" (sizeof (ldt)));
}
