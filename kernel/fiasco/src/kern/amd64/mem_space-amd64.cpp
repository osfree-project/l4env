INTERFACE [amd64]:

EXTENSION class Mem_space
{
private:
  void switch_ldt();

public:
  typedef Pml4 Dir_type;

protected:
  // DATA
  Dir_type *_dir;
};

IMPLEMENTATION[amd64]:

#include <cassert>
#include "l4_types.h"
#include "kmem.h"
#include "mem_unit.h"
#include "cpu_lock.h"
#include "lock_guard.h"
#include "paging.h"

#include <cstring>
#include "config.h"
#include "kmem.h"

PRIVATE static inline NEEDS ["cpu.h", "kmem.h"]
Pml4 *
Mem_space::current_pdir()
{
  return reinterpret_cast<Pml4*>(Kmem::phys_to_virt(Cpu::get_pdbr()));
}

IMPLEMENT inline NEEDS ["cpu.h", "kmem.h"]
void
Mem_space::make_current()
{
  _current = this;
  Cpu::set_pdbr((Mem_layout::pmem_to_phys(_dir)));
}


//---------------------------------------------------------------------------
IMPLEMENTATION [amd64-!smas]:

#include "cpu.h"
#include "mapped_alloc.h"
#include "kmem.h"
#include "logdefs.h"


IMPLEMENT inline NEEDS["kmem.h","logdefs.h"]
void
Mem_space::switchin_context()
{
  // never switch to kernel space (context of the idle thread)
  if (dir() == Kmem::dir())
    return;

  bool need_flush_tlb = false;
  Pdir *_pdir	      = _dir->lookup(Kmem::ipc_window(0))->pdp()
    				->lookup(Kmem::ipc_window(0))->pdir();
  unsigned index      = Pdir::virt_to_idx(Kmem::ipc_window(0));

  if ((*_pdir)[index] || (*_pdir)[index + 1])
    {
      (*_pdir)[index]     = 0;
      (*_pdir)[index + 1] = 0;
      need_flush_tlb  = true;
    }

  index = Pdir::virt_to_idx(Kmem::ipc_window(1));

  if ((*_pdir)[index] || (*_pdir)[index + 1])
    {
      (*_pdir)[index]     = 0;
      (*_pdir)[index + 1] = 0;
      need_flush_tlb  = true;
    }

  if (need_flush_tlb || dir() != current_pdir())
    {
      CNT_ADDR_SPACE_SWITCH;
      make_current();
      switch_ldt();
    }
}

//------------------------------------------------------------------------
IMPLEMENTATION [amd64-!segments-!smas]:

IMPLEMENT inline
void
Mem_space::switch_ldt()
{
}

IMPLEMENTATION [amd64]:

PUBLIC inline
void
Mem_space::enable_reverse_lookup()
{
  // Store reverse pointer to Mem_space in page directory
  (*_dir)[Mem_layout::Space_index] = reinterpret_cast<Unsigned64>(this);
}

/*
 * The following functions are all no-ops on native amd64.
 * Pages appear in an address space when the corresponding PTE is made
 * ... unlike Fiasco-UX which needs these special tricks
 */

IMPLEMENT inline
void
Mem_space::page_map (Address, Address, Address, unsigned)
{}

IMPLEMENT inline
void
Mem_space::page_protect (Address, Address, unsigned)
{}

IMPLEMENT inline
void
Mem_space::page_unmap (Address, Address)
{}

IMPLEMENT inline NEEDS ["kmem.h", "mapped_alloc.h"]
void Mem_space::kmem_update (void *addr)
{
  Unsigned64 attr = Pd_entry::Valid | Pd_entry::Writable |
		    Pd_entry::Referenced | Pd_entry::User;

  Pdir *kpdir = Kmem::dir()->lookup((Address)addr)
		->pdp()->lookup((Address)addr)
		  ->pdir();
  unsigned i = kpdir->virt_to_idx((Address)addr);
  
  Pdir *_pdir = _dir->lookup((Address)addr)
    ->alloc_pdp(Mapped_allocator::allocator(), attr)
    ->lookup((Address)addr)
    ->alloc_pdir(Mapped_allocator::allocator(), attr);
  
  (*_pdir)[i] = (*kpdir)[i];
}

/**
 * Copy multiple PDEs between two address spaces.
 * Be aware that some PDEs are reserved and contain thread-local information
 * like space_index, chief number, IO bitmap which must not be overwritten.
 * Callers are expected to know which slots are safe to copy.
 */
IMPLEMENT inline NEEDS [<cassert>, "mem_unit.h", "paging.h"]
void
Mem_space::remote_update (Address loc_addr, Mem_space const *rem,
                      Address rem_addr, size_t n)
{
  bool tlb_flush    = false;

  Pdir *rdir = 0;
  Pdir *ldir = 0;

  unsigned long loc_slot = Pdir::virt_to_idx(loc_addr);
  unsigned long rem_slot = Pdir::virt_to_idx(rem_addr);
  loc_addr &= ~((1UL << Pdp_entry::Shift)-1);
  rem_addr &= ~((1UL << Pdp_entry::Shift)-1);

  while (n--)
    {
      if (!ldir || ((loc_slot & 511) == 0))
	ldir = _dir->lookup(loc_addr + (loc_slot << Pd_entry::Shift))
	  ->pdp()->lookup(loc_addr + (loc_slot << Pd_entry::Shift))->pdir();

      if (!rdir || ((rem_slot & 511) == 0))
	rdir = rem->dir()->lookup(rem_addr + (rem_slot << Pd_entry::Shift))
	  ->pdp()->lookup(rem_addr + (rem_slot << Pd_entry::Shift))->pdir();

      if ((*ldir)[loc_slot & 511])
        tlb_flush = true;
      
      // XXX: If we copy a PDE and it maps either a superpage with PDE_GLOBAL
      // or a pagetable with one or multiple PTE_GLOBAL entries, these won't
      // be flushed on a TLB flush. Suggested solution: When mapping a page
      // with global bit, a PDE_NO_LONGIPC bit should be set in the
      // corresponding PDE and this bit should be checked here before copying.

      // This loop seems unnecessary, but remote_update is also used for
      // updating the long IPC window.
      // Now consider following scenario with super pages:
      // Sender A makes long IPC to receiver B.
      // A setups the IPC window by reading the pagedir slot from B in an 
      // temporary register. Now the sender is preempted by C. Then C unmaps 
      // the corresponding super page from B. C switch to A back, using 
      // switch_to, which clears the IPC window pde slots from A. BUT then A 
      // write the  content of the temporary register, which contain the now 
      // invalid pde slot, in his own page directory and starts the long IPC.
      // Because no pagefault will happen, A will write to now invalid memory.
      // So we compare after storing the pde slot, if the copy is still
      // valid. And this solution is much faster than grabbing the cpu lock,
      // when updating the ipc window.
      for(;;) {

	const volatile Pd_entry *val = rdir->index(rem_slot & 511);
	(*ldir)[loc_slot & 511] =  val->raw();
	
	if(EXPECT_TRUE((*ldir)[loc_slot & 511] == val->raw()))
	  break;
      }
      
      loc_slot++;
      rem_slot++;

    }

  if (tlb_flush)
    Mem_unit::tlb_flush();
}

// --------------------------------------------------------------------
IMPLEMENTATION[{ia32,amd64}-segments]:

PRIVATE inline
void
Mem_space::free_ldt_memory()
{
  if (ldt_addr())
    Mapped_allocator::allocator()->free(Config::PAGE_SHIFT,
				  reinterpret_cast<void*>(ldt_addr()));
}

// --------------------------------------------------------------------
IMPLEMENTATION[!smas]:

IMPLEMENT inline
void
Mem_space::update_small(Address, bool)
{}

