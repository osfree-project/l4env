INTERFACE:

EXTENSION class Space
{
public:

  void	page_map	(vm_offset_t phys, vm_offset_t virt,
                         vm_offset_t size, unsigned page_attribs);
  void	page_unmap	(vm_offset_t virt, vm_offset_t size);
  void	page_protect	(vm_offset_t virt, vm_offset_t size,
                         unsigned page_attribs);

  /** 
   * Update this address space with an entry from the kernel's shared 
   * address space.  The kernel maintains a 'master' page directory in 
   * class Kmem.  Use this function when an entry from the master directory 
   * needs to be copied into this address space.  
   * @param addr virtual address for which an entry should be copied from the 
   *             shared page directory.                                      
   */           
  void	kmem_update	(vm_offset_t addr);

  /**
   * Update a page in the small space window from Kmem.
   * @param flush true if TLB-Flush necessary
   */
  void	update_small	(vm_offset_t addr, bool flush);
};

IMPLEMENTATION[ia32-ux]:

#include <cstring>
#include "cpu.h"
#include "regdefs.h"

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
      pd_entry_t * iopde = _dir +
        ((get_virt_port_addr(0) >> PDESHIFT) & PDEMASK);
      if((*iopde & INTEL_PDE_VALID)
                                // this should never be a superpage
                                // but better be careful
         && !(*iopde & INTEL_PDE_SUPERPAGE))
        {                       // have a second level PT for IO bitmap
          // free the first half of the IO bitmap
          pt_entry_t * iopte = _dir + 
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

/** Insert a page-table entry, or upgrade an existing entry with new
    attributes.
    @param phys Physical address (page-aligned).
    @param virt Virtual address for which an entry should be created.
    @param size Size of the page frame -- 4KB or 4MB.
    @param page_attribs Attributes for the mapping (see
                        Space::page_attrib_t).
    @return Insert_ok if a new mapping was created;
             Insert_warn_exists if the mapping already exists;
             Insert_warn_attrib_upgrade if the mapping already existed but
                                        attributes could be upgraded;
             Insert_err_nomem if the mapping could not be inserted because
                              the kernel is out of memory;
             Insert_err_exists if the mapping could not be inserted because
                               another mapping occupies the virtual-address
                               range
    @pre phys and virt need to be size-aligned according to the size argument.
 */
PUBLIC Space::status_t
Space::v_insert(vm_offset_t phys, vm_offset_t virt, vm_offset_t size,
                  unsigned page_attribs)
{
  // insert page into page table

  // XXX should modify page table using compare-and-swap
  
  pd_entry_t *p = _dir + ((virt >> PDESHIFT) & PDEMASK);
  pt_entry_t *e;
  
  if (! (*p & INTEL_PDE_VALID)) // we don't have a page table for this area
    {
      // check if we can insert a 4MB mapping
      if (size == Config::SUPERPAGE_SIZE
          && (Cpu::features() & FEAT_PSE)
          && ((virt & ~Config::SUPERPAGE_MASK) == 0) 
	  && ((phys & ~Config::SUPERPAGE_MASK) == 0) )
        {
          *p = phys | INTEL_PDE_SUPERPAGE
            | INTEL_PDE_VALID | INTEL_PDE_REF
            | INTEL_PDE_MOD
            | page_attribs;

          page_map (phys, virt, size, page_attribs);

	  update_small(phys, false);

          return Insert_ok;
        }
         
      // can't map a superpage -- alloc new page table
      pt_entry_t *new_pt = (pt_entry_t *)Kmem_alloc::allocator()->alloc(0);
      if (! new_pt)
        return Insert_err_nomem;
      
      memset(new_pt, 0, Config::PAGE_SIZE);
      
      *p = Kmem::virt_to_phys(new_pt)
        | INTEL_PDE_VALID | INTEL_PDE_WRITE
        | INTEL_PDE_REF | INTEL_PDE_USER;  

      update_small(phys, false);

      e = new_pt + ((virt >> PTESHIFT) & PTEMASK);
    }
  else if (*p & INTEL_PDE_SUPERPAGE)
    {
      // already have mapped a superpage
      
      if ((*p & Config::SUPERPAGE_MASK) | (virt & ~Config::SUPERPAGE_MASK) != phys)
        return Insert_err_exists;

      // same mapping -- check for page-attribute upgrade
      if ((*p | page_attribs) == *p) // no attrib change?
        return Insert_warn_exists;

      // change attributes
      
      if (size == Config::SUPERPAGE_SIZE)
        {
          // upgrade the whole superpage at once
          *p |= page_attribs;
          
          page_protect (virt, size, page_attribs);

	  update_small(phys, false);

          return Insert_warn_attrib_upgrade;
        }

      // we need to split the superpage mapping because
      // it's only partly upgraded
      
      pt_entry_t *new_pt = split_pgtable(p);
      if (! new_pt)
        return Insert_err_nomem;
      
      *p = Kmem::virt_to_phys(new_pt)
        | INTEL_PDE_VALID | INTEL_PDE_WRITE
        | INTEL_PDE_REF | INTEL_PDE_USER;  

      update_small(phys, false);
      
      e = new_pt + ((virt >> PTESHIFT) & PTEMASK);
    }
  else if (size == Config::SUPERPAGE_SIZE)
    {
      return Insert_err_exists;
    }
  else
    { 
      e = static_cast<pt_entry_t *>(Kmem::phys_to_virt(*p & Config::PAGE_MASK))
        + ((virt >> PTESHIFT) & PTEMASK);
    }

  assert(size == Config::PAGE_SIZE);
  
  if (*e & INTEL_PTE_VALID)     // anything mapped already?
    {
      if ((*e & Config::PAGE_MASK) != phys)
        return Insert_err_exists;
      
      if ((*e | page_attribs) == *e) // no attrib change?
        return Insert_warn_exists;

      // upgrade from read-only to read-write
      *e |= page_attribs;

      page_protect (virt, size, page_attribs);

      return Insert_warn_attrib_upgrade;
    }
  else                  // we don't have mapped anything
    {
      *e = phys | INTEL_PTE_MOD | INTEL_PTE_VALID | INTEL_PTE_REF
        | page_attribs;

      page_map (phys, virt, size, page_attribs);
    }

  return Insert_ok;
}

/** Look up a page-table entry.
    @param virt Virtual address for which we try the look up.
    @param phys Meaningful only if we find something (and return true).
                If not 0, we fill in the physical address of the found page
                frame.
    @param page_attribs Meaningful only if we find something (and return true).
                If not 0, we fill in the page attributes for the found page    
                frame (see Space::page_attrib_t).
    @param size If not 0, we fill in the size of the page-table slot.  If an
                entry was found (and we return true), this is the size
                of the page frame.  If no entry was found (and we
                return false), this is the size of the free slot.  In
                either case, it is either 4KB or 4MB.
    @return True if an entry was found, false otherwise.
 */
PUBLIC bool
Space::v_lookup(vm_offset_t virt, vm_offset_t *phys = 0,
                  vm_offset_t *size = 0, unsigned *page_attribs = 0)
{
  const pd_entry_t *p = _dir + ((virt >> PDESHIFT) & PDEMASK);

  if (! (*p & INTEL_PDE_VALID))
    {
      if (size) *size = Config::SUPERPAGE_SIZE;
      return false;
    }

  if (*p & INTEL_PDE_SUPERPAGE)
    {
      if (phys) *phys = *p & Config::SUPERPAGE_MASK;
      if (size) *size = Config::SUPERPAGE_SIZE;
      if (page_attribs) *page_attribs = (*p & Page_all_attribs);

      return true;
    }

  const pt_entry_t *e = static_cast<pt_entry_t *>
    (Kmem::phys_to_virt(*p & Config::PAGE_MASK))
    + ((virt >> PTESHIFT) & PTEMASK);

  if (size) *size = Config::PAGE_SIZE;

  if (! (*e & INTEL_PTE_VALID))
    return false;

  if (phys) *phys = *e & Config::PAGE_MASK;
  if (page_attribs) *page_attribs = (*e & Page_all_attribs);

  return true;
}

/** Delete a page-table entries, or some of the entries' attributes.  This 
    function works for one or multiple mappings (in contrast to v_insert!). 
    @param virt Virtual address of the memory region that should be changed.
    @param size Size of the memory region that should be changed.
    @param page_attribs If nonzero, delete only the given page attributes.
                        Otherwise, delete the whole entries.
    @return False if an error occurs and some of the mappings could
             not be flushed.  True if all goes well.
 */
PUBLIC bool 
Space::v_delete(vm_offset_t virt, vm_offset_t size,
                  unsigned page_attribs = 0)
{
  // delete page from page table
  
  for (vm_offset_t va = virt;
       va < virt + size;
       va += Config::PAGE_SIZE)
    {
      pd_entry_t *p = _dir + ((va >> PDESHIFT) & PDEMASK);
      pt_entry_t *e;
      
      if (! (*p & INTEL_PDE_VALID))
        {
          // no page dir entry -- warp to next page dir entry
          va = ((va + Config::SUPERPAGE_SIZE) & Config::SUPERPAGE_MASK)  
	    - Config::PAGE_SIZE;

          if (Config::conservative)
            kdb_ke("v_delete unmapped pgtable");

          continue;
        }

      if (*p & (INTEL_PDE_SUPERPAGE | Kmem::pde_global()))
        {
          // oops, a superpage or a shared ptable; see if we can unmap
          // it at once, otherwise split it

          if (((va & ~Config::SUPERPAGE_MASK) == 0) // superpage boundary
              && va + Config::SUPERPAGE_SIZE <= virt + size)
            {
              // unmap superpage
              if (page_attribs)
                *p &= ~page_attribs;
              else
                *p = 0;
              va += Config::SUPERPAGE_SIZE - Config::PAGE_SIZE;

	      update_small(va, true);

              continue;
            }

          // need to unshare/split

          pt_entry_t *new_pt = split_pgtable(p);
          if (! new_pt)  
            return false;
          
          *p = Kmem::virt_to_phys(new_pt)
            | INTEL_PDE_VALID | INTEL_PDE_WRITE
            | INTEL_PDE_REF | INTEL_PTE_USER;

	  update_small(va, true);
	  
          e = new_pt + ((va >> PTESHIFT) & PTEMASK);
        } 
      else
        e = static_cast<pt_entry_t *>(Kmem::phys_to_virt(*p & Config::PAGE_MASK))
          + ((va >> PTESHIFT) & PTEMASK);

      if (Config::conservative && !(*e & INTEL_PTE_VALID))
        kdb_ke("v_delete unmapped page");

      if (page_attribs)
        *e &= ~page_attribs;
      else
        *e = 0;                 // delete page table entry

      page_unmap (va, size);
    }

  return true;
}

static pt_entry_t *
split_pgtable(pd_entry_t *p)
{
  // alloc new page table
  pt_entry_t *new_pt = static_cast<pt_entry_t *>(Kmem_alloc::allocator()->alloc(0));
  if (! new_pt)
    return 0;  
               
  if (*p & INTEL_PDE_SUPERPAGE)
    {
      for (unsigned i = 0; i < 1024; i++)
        {
          new_pt[i] = ((*p & Config::SUPERPAGE_MASK) + (i << Config::PAGE_SHIFT))
            | INTEL_PTE_MOD | INTEL_PTE_VALID | INTEL_PTE_REF 
            | (*p & Space::Page_all_attribs);
        }
    }
  else
    { 
      std::memcpy(new_pt, p, Config::PAGE_SIZE);
    }

  return new_pt;
}
