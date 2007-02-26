/*
 * Fiasco-UX
 * Emulation Code for some processor internals (registers, instructions)
 */

INTERFACE:

// Someone should really get rid of this
#include <flux/x86/seg.h>		// for pseudo_descriptor
#include "linker_syms.h"

class Emulation
{
public:
  static unsigned	get_pdir_addr	(void);
  static void		set_pdir_addr	(unsigned addr);
  static unsigned	get_fault_addr	(void);
  static void		set_fault_addr	(unsigned addr);
  static unsigned	get_idt_base	(void);
  static void		lidt		(pseudo_descriptor *desc);
  static unsigned short	get_kernel_cs	(void);
  static unsigned short	get_kernel_ss	(void);

  static const unsigned trampoline_frame = 0x1000;
  static const unsigned trampoline_page  = 
                        reinterpret_cast<unsigned>(&_trampoline_page);

private:
  static unsigned	page_dir_addr	asm ("PAGE_DIR_ADDR");
  static unsigned	page_fault_addr	asm ("PAGE_FAULT_ADDR");
  static unsigned long	idt_base;
  static unsigned short	idt_limit;
};

IMPLEMENTATION:

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
 * Return IDT base address
 * @return IDT base address
 */
IMPLEMENT inline
unsigned
Emulation::get_idt_base()
{
  return idt_base;
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
