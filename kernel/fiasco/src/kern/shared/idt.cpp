/*
 * Fiasco Interrupt Descriptor Table (IDT) Code
 */

INTERFACE:

#include "initcalls.h"
#include "kmem.h"
#include "mem_layout.h"
#include "types.h"
#include "x86desc.h"

class Idt_entry : public X86desc
{
};

class Idt_init_entry;

class Idt
{
  friend class Jdb_kern_info_bench;
public:
  // idt entries for 0x20 CPU exceptions, 0x10 IRQs, 7 syscalls,
  // 0x3e/0x3f for APIC exceptions
  static const unsigned _idt_max = 0x40;
private:
  static const Address  _idt = Mem_layout::Idt;
};

IMPLEMENTATION:

#include <cassert>
#include "gdt.h"
#include "idt_init.h"
#include "mem_unit.h"
#include "paging.h"
#include "panic.h"
#include "vmem_alloc.h"

PUBLIC inline
Idt_entry::Idt_entry()
{}

// interrupt gate
PUBLIC inline
Idt_entry::Idt_entry(Address entry, Unsigned16 selector, Unsigned8 access)
{
  _data.i._offset_low  = entry & 0x0000ffff;
  _data.i._selector    = selector;
  _data.i._zero        = 0;
  _data.i._access      = access | Access_present;
  _data.i._offset_high = (entry & 0xffff0000) >> 16;
}

// task gate
PUBLIC inline
Idt_entry::Idt_entry(Unsigned16 selector, Unsigned8 access)
{
  _data.t._selector    = selector;
  _data.t._access      = access | Access_present;
  _data.t._avail1      = 0;
  _data.t._avail2      = 0;
  _data.t._avail3      = 0;
}

PUBLIC inline
void
Idt_entry::clear()
{
  _data.r = 0;
}

PUBLIC inline
Address
Idt_entry::offset()
{
  return idt_entry_offset();
}

PUBLIC inline
Unsigned8
Idt_entry::word_count()
{
  return _data.i._zero;
}

/**
 * IDT write-protect/write-unprotect function.
 * @param writable true if IDT should be made writable, false otherwise
 */
PRIVATE static
void
Idt::set_writable (bool writable)
{
  Pd_entry p = Kmem::dir()->entry(_idt);

  // Make sure page directory entry is valid and not a 4MB page
  assert (p.valid() && !p.superpage());

  Pt_entry *e = p.ptab()->lookup(_idt);

  // Make sure page table entry is valid
  assert (e->valid());

  if (writable)
    e->add_attr(Pt_entry::Writable); // Make read-write
  else
    e->del_attr(Pt_entry::Writable); // Make read-only

  Mem_unit::tlb_flush (_idt);
}

PUBLIC static FIASCO_INIT
void
Idt::init_table(Idt_init_entry *src)
{
  Idt_entry *entries = (Idt_entry*)_idt;

  while (src->entry)
    {
      entries[src->vector] = 
	((src->type & 0x1f) == 0x05) // task gate?
	  ? Idt_entry(src->entry, src->type)
	  : Idt_entry(src->entry, Gdt::gdt_code_kernel, src->type);
      src++;
    }
}

/**
 * IDT initialization function. Sets up initial interrupt vectors.
 * It also write-protects the IDT because of the infamous Pentium F00F bug.
 */
PUBLIC static FIASCO_INIT
void
Idt::init()
{
  if (!Vmem_alloc::page_alloc ((void *) _idt, Vmem_alloc::ZERO_FILL))
    panic ("IDT allocation failure");

  init_table((Idt_init_entry*)&idt_init_table);

  Pseudo_descriptor desc(_idt, _idt_max*sizeof(Idt_entry)-1);
  set (&desc);

  set_writable (false);
}

/**
 * IDT patching function.
 * Allows to change interrupt gate vectors at runtime.
 * It makes the IDT writable for the duration of this operation.
 * @param vector interrupt vector to be modified
 * @param func new handler function for this interrupt vector
 * @param user true if user mode can use this vector, false otherwise
 */
PUBLIC static
void
Idt::set_entry(unsigned vector, Address entry, bool user)
{
  assert (vector < _idt_max);

  set_writable (true);

  Idt_entry *entries = (Idt_entry*)_idt;
  if (entry)
    entries[vector] = Idt_entry(entry, Gdt::gdt_code_kernel,
			        Idt_entry::Access_intr_gate |
			        (user ? Idt_entry::Access_user 
			              : Idt_entry::Access_kernel));
  else
    entries[vector].clear();

  set_writable (false);
}

PUBLIC static inline
Address
Idt::idt()
{
  return _idt;
}


//---------------------------------------------------------------------------
IMPLEMENTATION[ia32]:

#include "config.h"
#include "timer.h"

/**
 * IDT loading function.
 * Loads IDT base and limit into the CPU.
  * @param desc IDT descriptor (base address, limit)
  */  
PUBLIC static inline
void
Idt::set (Pseudo_descriptor *desc)
{
  asm volatile ("lidt %0" : : "m" (*desc));
}

PUBLIC static inline
void
Idt::get (Pseudo_descriptor *desc)
{
  asm volatile ("sidt %0" : "=m" (*desc) : : "memory");
}

extern "C" void entry_int_timer(void);
extern "C" void entry_int_timer_slow(void);
extern "C" void entry_int_timer_stop(void);
extern "C" void entry_int7(void);
extern "C" void entry_intf(void);
extern "C" void entry_int_pic_ignore(void);

/**
 * Set IDT vector to the normal timer interrupt handler.
 */
PUBLIC static
void
Idt::set_vectors_run(void)
{
  Address func = (Config::esc_hack || Config::watchdog ||
		  Config::serial_esc==Config::SERIAL_ESC_NOIRQ)
		    ? (Address)entry_int_timer_slow // slower for debugging
		    : (Address)entry_int_timer;     // non-debugging

  set_entry (Config::scheduler_irq_vector, func, false);
  set_entry (0x27, (Address) entry_int7, false);
  set_entry (0x2f, (Address) entry_intf, false);
}

/**
 * Set IDT vector to a dummy vector if Config::getchar_does_hlt is true.
 */
PUBLIC static
void
Idt::set_vectors_stop(void)
{
  // acknowledge timer interrupt once to keep timer interrupt alive because
  // we could be called from thread_timer_interrupt_slow() before ack
  Timer::acknowledge();

  // set timer interrupt to dummy doing nothing
  set_entry (Config::scheduler_irq_vector, 
	     (Address) entry_int_timer_stop, false);

  // From ``8259A PROGRAMMABLE INTERRUPT CONTROLLER (8259A 8259A-2)'': If no
  // interrupt request is present at step 4 of either sequence (i. e. the
  // request was too short in duration) the 8259A will issue an interrupt
  // level 7. Both the vectoring bytes and the CAS lines will look like an
  // interrupt level 7 was requested.
  set_entry (0x27, (Address) entry_int_pic_ignore, false);
  set_entry (0x2f, (Address) entry_int_pic_ignore, false);
}


//---------------------------------------------------------------------------
IMPLEMENTATION[ux]:

#include "emulation.h"

PUBLIC static
void
Idt::set (Pseudo_descriptor *desc)
{
  Emulation::lidt (desc);
}

