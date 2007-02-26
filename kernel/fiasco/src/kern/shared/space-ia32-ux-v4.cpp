INTERFACE:

#include "l4_types.h"

IMPLEMENTATION[ia32-ux-v4]:

#include "atomic.h"

/** Destructor.  Deletes the address space.
 */
PUBLIC
Space::~Space()
{
  // free all page tables we have allocated for this address space
  // except the ones in kernel space which are always shared

  for (unsigned i = 0; i < (Kmem::mem_user_max >> Config::SUPERPAGE_SHIFT); i++)
    if ((_dir[i] & INTEL_PDE_VALID) &&
       !(_dir[i] & (INTEL_PDE_SUPERPAGE | Kmem::pde_global())))
      Kmem_alloc::allocator()->free(0,P_ptr<void>(_dir[i] & Config::PAGE_MASK));

  // free IO bitmap (if allocated)

  if(Config::enable_io_protection)
    {
      Pd_entry * iopde = _dir +
        ((get_virt_port_addr(0) >> PDESHIFT) & PDEMASK);
      if((*iopde & INTEL_PDE_VALID)
                                // this should never be a superpage
                                // but better be careful
         && !(*iopde & INTEL_PDE_SUPERPAGE))
        {                       // have a second level PT for IO bitmap
          // free the first half of the IO bitmap
          Pt_entry * iopte = _dir + 
            ((get_virt_port_addr(0) >> PTESHIFT) & PTEMASK);
          if(*iopte & INTEL_PTE_VALID)
            Kmem_alloc::allocator()->free(0,P_ptr<void>(*iopte & Config::PAGE_MASK));

          // XXX invalidate entries ??

          // free the second half
          iopte = _dir + 
            ((get_virt_port_addr(L4_fpage::IO_PORT_MAX / 2) >> PTESHIFT) & PTEMASK);
          if(*iopte & INTEL_PTE_VALID)
            Kmem_alloc::allocator()->free(0,P_ptr<void>(*iopte & Config::PAGE_MASK));

          // free the page table
          Kmem_alloc::allocator()->free(0,P_ptr<void>(*iopde & Config::PAGE_MASK));
        }
    }
}


/** Thread counter.
 * @return number of threads existing in this space
 */
PUBLIC inline 
Mword Space::thread_count() {
  // counter is right-shifted by 1 since it mustn't get INTEL_PDE_VALID
  return _dir[number_index] >> 1;
}

/** Add thread.
 * Increases the thread counter for this space by 1.
 * @return the new count of threads
 */
PUBLIC inline NEEDS ["atomic.h"]
Mword Space::thread_add() {
  atomic_add ((Mword*)&(_dir[number_index]), (Mword)(1 << 1));
  return thread_count();
}

/** Remove thread.
 * Decreases the thread counter for this space by 1.
 * @return the new count of threads
 */
PUBLIC inline NEEDS ["atomic.h"]
Mword Space::thread_remove() {
  atomic_sub ((Mword*)&(_dir[number_index]), (Mword)(1 << 1));
  return thread_count();
}

/** KIP address
 * @return address where the KIP is mapped in current space
 */
PUBLIC inline NEEDS ["l4_types.h"]
Address Space::kip_address() {
  return L4_fpage (_dir[kip_index]).page();
}

/** UTCB area
 * @return the utcb area Fpage in current space
 */
PUBLIC inline NEEDS ["l4_types.h"]
L4_fpage Space::utcb_area() {
  return L4_fpage (_dir[chief_index]);
}
