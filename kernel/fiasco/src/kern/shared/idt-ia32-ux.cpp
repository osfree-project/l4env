/*
 * Fiasco IDT Code
 * Shared between UX and native IA32.
 */

INTERFACE:

#include <flux/x86/seg.h>
#include "initcalls.h"
#include "kmem.h"
#include "types.h"

class Idt
{
public:

/**
  * IDT initialization function. Sets up initial interrupt gate vectors.
  * It also write-protects the IDT because of the infamous Pentium F00F bug.
  */
  static void init() FIASCO_INIT;
  
/**
  * IDT loading function. Loads IDT base and limit into the CPU and thus
  * makes the IDT known to the hardware.
  * @param desc IDT descriptor (base address, limit)
  */  
  static void load_idt (pseudo_descriptor *desc) FIASCO_INIT;
  
/**
  * IDT patching function. Allows to change interrupt gate vectors at runtime.
  * It makes the IDT writable for the duration of this operation.
  * @param vector interrupt vector to be modified
  * @param func new handler function for this interrupt vector
  * @param user true if user mode can use this vector, false otherwise
  */
  static void set_vector (unsigned vector, Address func, bool user);
  
/**
  * Virtual address of the Interrupt Descriptor Table (IDT)
  */  
  static const Address idt = (Address) Kmem::_idt_addr;

private:

/**
  * IDT write-protect/write-unprotect function.
  * @param writable true if IDT should be made writable, false otherwise
  */
  static void set_writable (bool writable);
};

IMPLEMENTATION[ia32-ux]:

#include <flux/x86/gate_init.h>
#include <flux/x86/paging.h>
#include <cassert>
#include "entry.h"
#include "kmem.h"
#include "panic.h"
#include "vmem_alloc.h"

IMPLEMENT
void
Idt::set_writable (bool writable)
{
  const Pd_entry *pde = Kmem::dir() + ((idt >> PDESHIFT) & PDEMASK);

  // Make sure page directory entry is valid and not a 4MB page
  assert ((*pde & INTEL_PDE_VALID) && !(*pde & INTEL_PDE_SUPERPAGE));

  Pt_entry *pte = static_cast<Pt_entry *>
                    (Kmem::phys_to_virt (*pde & Config::PAGE_MASK)) +
                    ((idt >> PTESHIFT) & PTEMASK);

  // Make sure page table entry is valid
  assert ((*pte & INTEL_PTE_VALID));

  if (writable)
    *pte |= INTEL_PTE_WRITE;		// Make read-write
  else
    *pte &= ~INTEL_PTE_WRITE;		// Make read-only

  Kmem::tlb_flush (idt);
}

IMPLEMENT
void
Idt::init()
{
  if (!Vmem_alloc::page_alloc ((void *) idt, 0, Vmem_alloc::ZERO_FILL))
    panic ("IDT allocation failure");

  pseudo_descriptor desc;
  desc.limit       = idt_max * 8 - 1;
  desc.linear_base = idt;

  gate_init ((x86_gate *) idt, idt_init_table, Kmem::gdt_code_kernel);
  load_idt (&desc);

  set_writable (false);
}

IMPLEMENT
void
Idt::set_vector (unsigned vector, Address func, bool user)
{
  assert (vector < idt_max);

  set_writable (true);

  fill_gate ((x86_gate *) idt + vector, func,
             Kmem::gdt_code_kernel,
             ACC_INTR_GATE | (user ? ACC_PL_U : ACC_PL_K), 0);

  set_writable (false);
}

