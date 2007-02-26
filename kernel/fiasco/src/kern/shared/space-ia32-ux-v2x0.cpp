IMPLEMENTATION[ia32-ux-v2x0]:

/** Destructor.  Deletes the address space and unregisters it from
    Space_index.
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

  // deregister from task table
  Space_index::del(Space_index(_dir[number_index] >> 8),
                   Space_index(_dir[chief_index]  >> 8));
}

PUBLIC inline
Address Space::kip_address() {
  return is_sigma0() ? 
    Kmem::virt_to_phys(Kmem::info())
    : Config::AUTO_MAP_KIP_ADDRESS;
}

