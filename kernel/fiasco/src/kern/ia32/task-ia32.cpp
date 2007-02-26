IMPLEMENTATION [ia32]:

IMPLEMENT
Task::~Task()
{
  cleanup();
}

//------------------------------------------------------------------------
IMPLEMENTATION [ia32-utcb]:

IMPLEMENT inline void Task::map_utcb_ptr_page() {}

//------------------------------------------------------------------------
IMPLEMENTATION [ia32-v2-utcb]:

#include "paging.h"
#include "mem_layout.h"
#include "mapped_alloc.h"

IMPLEMENT
void
Task::free_utcb_pagetable()
{
  Address utcb_start_addr = Mem_layout::V2_utcb_addr;

  // Mapped in userland
  if (utcb_start_addr < Kmem::mem_user_max)
    return;

  Pd_entry *utcb_pde = _dir.lookup(utcb_start_addr);

  Address ptab_addr = utcb_pde->ptabfn();
  *utcb_pde = 0;

  // free the page table
  Mapped_allocator::allocator()->free_phys(Config::PAGE_SHIFT,
                                     P_ptr <void> (ptab_addr));
}
