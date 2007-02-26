/*
 * Fiasco-ia32
 * Architecture specific pagetable code
 */

IMPLEMENTATION[ia32]:

/** Constructor.  Creates a new address space and registers it with
  * Space_index.
  *
  * Registration may fail (if a task with the given number already
  * exists, or if another thread creates an address space for the same
  * task number concurrently).  In this case, the newly-created
  * address space should be deleted again.
  *
  * @param new_number Task number of the new address space
  */
PROTECTED
Space::Space(unsigned new_number)
{
  Kmem::dir_init(_dir);		// copy current shared kernel page directory

  // scribble task/chief numbers into an unused part of the page directory
  _dir[number_index] = new_number << 8;		// must not be INTEL_PDE_VALID
  _dir[chief_index] = Space_index(new_number).chief() << 8; // dito

  Space_index::add(this, new_number);		// register in task table
}

/*
 * The following functions are all no-ops on native ia32.
 * Pages appear in an address space when the corresponding PTE is made
 * ... unlike Fiasco-UX which needs these special tricks
 */

IMPLEMENT inline
void
Space::page_map (vm_offset_t, vm_offset_t, vm_offset_t, unsigned)
{}

IMPLEMENT inline
void
Space::page_protect (vm_offset_t, vm_offset_t, unsigned)
{}

IMPLEMENT inline
void
Space::page_unmap (vm_offset_t, vm_offset_t)
{}

IMPLEMENT inline
void  
Space::kmem_update (vm_offset_t addr)
{
  unsigned i = (addr >> PDESHIFT) & PDEMASK;
  _dir[i] = Kmem::dir()[i];
}

/* Update a page in the small space window from kmem.
 * @param flush true if TLB-Flush necessary
 */
IMPLEMENT
void
Space::update_small( vm_offset_t addr, bool flush)
{
#ifdef CONFIG_SMALL_SPACES
    {
      if (is_small_space() &&
	  addr < small_space_size() )
	{
	  unsigned version;

	  version = Kmem::update_smas_window( small_space_base() + addr,
					     _dir[(addr >> PDESHIFT) & PDEMASK],
					     flush );

	  current_space()->kmem_update(small_space_base() + addr);
	  current_space()->set_pdir_version ( version );
	}
    }
#else
  (void)addr; (void)flush; // prevent gcc warning
#endif
}
