INTERFACE:

EXTENSION class Space
{
public:

  void	page_map	(Address phys, Address virt,
                         Address size, unsigned page_attribs);
                         
  void	page_unmap	(Address virt, Address size);
  
  void	page_protect	(Address virt, Address size,
                         unsigned page_attribs);

  /** 
   * Update this address space with an entry from the kernel's shared 
   * address space.  The kernel maintains a 'master' page directory in 
   * class Kmem.  Use this function when an entry from the master directory 
   * needs to be copied into this address space.  
   * @param addr virtual address for which an entry should be copied from the 
   *             shared page directory.                                      
   */           
  void kmem_update (void *addr);

  /**
   * Populate this address space with n PDEs from a remote address space.
   * Use this function when a part of another address space needs to be
   * temporarily visible in the current address space (i.e., for long IPC).
   * If one of the PDEs was valid, i.e. non-null, we need a TLB flush.
   * @param loc_addr virtual address where temporary should be established.
   * @param rem remote address space.
   * @param rem_addr virtual address in remote address space.
   * @param n number of PDEs to copy.
   */
  void remote_update (const Address, const Space_context *,
                      const Address, size_t);

  /**
   * Update a page in the small space window from Kmem.
   * @param flush true if TLB-Flush necessary
   */
  void update_small (Address addr, bool flush);
};

IMPLEMENTATION[ia32-ux]:

#include <cstring>
#include "cpu.h"
#include "kdb_ke.h"
#include "regdefs.h"
#include "std_macros.h"

/** Insert a page-table entry, or upgrade an existing entry with new
    attributes.
    @param phys Physical address (page-aligned).
    @param virt Virtual address for which an entry should be created.
    @param size Size of the page frame -- 4KB or 4MB.
    @param page_attribs Attributes for the mapping (see
                        Space::Page_attrib).
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
PUBLIC
Space::Status Space::v_insert (Address phys, Address virt, size_t size,
                               unsigned page_attribs)
{
  // insert page into page table

  // XXX should modify page table using compare-and-swap
  
  Pd_entry *p = _dir + ((virt >> PDESHIFT) & PDEMASK);
  Pt_entry *e;
  
  // This 4 MB area is not mapped
  if (EXPECT_TRUE (!(*p & INTEL_PDE_VALID)))
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
      Pt_entry *new_pt = (Pt_entry *)Kmem_alloc::allocator()->alloc(0);

      if (EXPECT_FALSE (!new_pt))
        return Insert_err_nomem;
      
      memset(new_pt, 0, Config::PAGE_SIZE);
      
      *p = Kmem::virt_to_phys(new_pt)
        | INTEL_PDE_VALID | INTEL_PDE_WRITE
        | INTEL_PDE_REF | INTEL_PDE_USER;  

      update_small(phys, false);

      e = new_pt + ((virt >> PTESHIFT) & PTEMASK);
    }

  // This 4 MB area is covered by a superpage
  else if (EXPECT_TRUE (*p & INTEL_PDE_SUPERPAGE))
    {
      // want to change physical attributes?
      if (EXPECT_FALSE ((  (*p &    Config::SUPERPAGE_MASK)
			 | (virt & ~Config::SUPERPAGE_MASK)) != phys) )
        return Insert_err_exists;

      // same mapping -- check for page-attribute upgrade
      if (EXPECT_FALSE ((*p | page_attribs) == *p)) // no attrib change?
        return Insert_warn_exists;

      // change attributes?
      if (size == Config::SUPERPAGE_SIZE)
        {
          // upgrade the whole superpage at once
          *p |= page_attribs;
          
          page_protect (virt, size, *p & Page_all_attribs);

	  update_small(phys, false);

          return Insert_warn_attrib_upgrade;
        }

      // we need to split the superpage mapping because
      // it's only partly upgraded

      Pt_entry *new_pt = split_pgtable(p);
      if (EXPECT_FALSE (! new_pt) )
        return Insert_err_nomem;
      
      *p = Kmem::virt_to_phys(new_pt)
        | INTEL_PDE_VALID | INTEL_PDE_WRITE
        | INTEL_PDE_REF | INTEL_PDE_USER;  

      update_small(phys, false);
      
      e = new_pt + ((virt >> PTESHIFT) & PTEMASK);
    }

  // This 4 MB area is covered by a pagetable
  else if (EXPECT_FALSE (size == Config::SUPERPAGE_SIZE))
    return Insert_err_exists;

  else
    e = static_cast<Pt_entry *>(Kmem::phys_to_virt (*p & Config::PAGE_MASK))
      + ((virt >> PTESHIFT) & PTEMASK);

  assert (size == Config::PAGE_SIZE);
  
  // anything mapped already?
  if (EXPECT_FALSE (*e & INTEL_PTE_VALID))
    {
      // different page?
      if (EXPECT_FALSE ((*e & Config::PAGE_MASK) != phys))
        return Insert_err_exists;
      
      // no attrib change?
      if (EXPECT_FALSE ((*e | page_attribs) == *e))
        return Insert_warn_exists;

      // upgrade from read-only to read-write
      *e |= page_attribs;

      page_protect (virt, size, *e & Page_all_attribs);

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
                frame (see Space::Page_attrib).
    @param size If not 0, we fill in the size of the page-table slot.  If an
                entry was found (and we return true), this is the size
                of the page frame.  If no entry was found (and we
                return false), this is the size of the free slot.  In
                either case, it is either 4KB or 4MB.
    @return True if an entry was found, false otherwise.
 */
PUBLIC
bool Space::v_lookup (Address virt, Address *phys = 0, Address *size = 0,
                      unsigned *page_attribs = 0)
{
  const Pd_entry *p = _dir + ((virt >> PDESHIFT) & PDEMASK);

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

  const Pt_entry *e = static_cast<Pt_entry *>
    (Kmem::phys_to_virt(*p & Config::PAGE_MASK))
    + ((virt >> PTESHIFT) & PTEMASK);

  if (size) *size = Config::PAGE_SIZE;

  if (! (*e & INTEL_PTE_VALID))
    return false;

  if (phys) *phys = *e & Config::PAGE_MASK;
  if (page_attribs) *page_attribs = (*e & Page_all_attribs);

  return true;
}

/** Delete page-table entries, or some of the entries' attributes.  This 
    function works for one or multiple mappings (in contrast to v_insert!). 
    @param virt Virtual address of the memory region that should be changed.
    @param size Size of the memory region that should be changed.
    @param page_attribs If nonzero, delete only the given page attributes.
                        Otherwise, delete the whole entries.
    @return False if an error occurs and some of the mappings could
             not be flushed.  True if all goes well.
 */
PUBLIC
bool Space::v_delete (Address virt,
                      Address size,
                      unsigned page_attribs = 0)
{
  // delete pages from page tables
  
  for (Address va = virt; va < virt + size; va += Config::PAGE_SIZE)
    {
      Pd_entry *p = _dir + ((va >> PDESHIFT) & PDEMASK);
      Pt_entry *e;
      
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
              if (page_attribs)		
                {
                  *p &= ~page_attribs;	// downgrade PDE (superpage) rights
                  page_protect (va, size, *p & Page_all_attribs);
                }
              else
                {
                  *p = 0;		// delete PDE (superpage)
                  page_unmap (va, size);
                }
              
              va += Config::SUPERPAGE_SIZE - Config::PAGE_SIZE;

	      update_small(va, true);

              continue;
            }

          // need to unshare/split

          Pt_entry *new_pt = split_pgtable(p);
          if (!new_pt)
            return false;
          
          *p = Kmem::virt_to_phys(new_pt)
            | INTEL_PDE_VALID | INTEL_PDE_WRITE
            | INTEL_PDE_REF | INTEL_PTE_USER;

	  update_small(va, true);
	  
          e = new_pt + ((va >> PTESHIFT) & PTEMASK);
        } 

      else
        e = static_cast<Pt_entry *>(Kmem::phys_to_virt(*p & Config::PAGE_MASK))
          + ((va >> PTESHIFT) & PTEMASK);

      if (Config::conservative && !(*e & INTEL_PTE_VALID))
        kdb_ke("v_delete unmapped page");

      if (page_attribs)
        {
          *e &= ~page_attribs;		// downgrade PTE rights
          page_protect (va, size, *e & Page_all_attribs);
        }
      else
        {
          *e = 0;			// delete PTE
          page_unmap (va, size);
        }
    }

  return true;
}

static Pt_entry * split_pgtable (Pd_entry *p)
{
  // alloc new page table
  Pt_entry *new_pt = static_cast<Pt_entry *>(Kmem_alloc::allocator()->alloc(0));

  if (!new_pt)
    return 0;  
               
  if (*p & INTEL_PDE_SUPERPAGE)
    for (unsigned i = 0; i < 1024; i++)
      new_pt[i] = ((*p & Config::SUPERPAGE_MASK) + (i << Config::PAGE_SHIFT))
                | INTEL_PTE_MOD | INTEL_PTE_VALID | INTEL_PTE_REF 
                | (*p & Space::Page_all_attribs);

  else
    std::memcpy (new_pt, p, Config::PAGE_SIZE);

  return new_pt;
}
