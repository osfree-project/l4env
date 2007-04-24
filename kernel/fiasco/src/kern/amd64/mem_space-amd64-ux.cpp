INTERFACE [ux,amd64]:

EXTENSION class Mem_space
{
public:

  void	page_map	(Address phys, Address virt,
                         Address size, unsigned page_attribs);
                         
  void	page_unmap	(Address virt, Address size);
  
  void	page_protect	(Address virt, Address size,
                         unsigned page_attribs);

  /**
   * Get KIP address.
   * @return Address there the KIP is mapped.
   */
  Address kip_address() const;

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
  void remote_update (Address, Mem_space const *, Address, size_t);

  /**
   * Update a page in the small space window from Kmem.
   * @param flush true if TLB-Flush necessary
   */
  void update_small (Address virt, bool flush);

  /**
   * Check if an memory region may be unmapped or overmapped.
   *
   * In V2-utcb, it returns false if the memory region overlaps the utcb area
   */
  bool is_mappable (Address addr, size_t size);
};

IMPLEMENTATION [ux,amd64]:

#include <cstring>
#include <cstdio>
#include "cpu.h"
#include "kdb_ke.h"
#include "mapped_alloc.h"
#include "std_macros.h"


PUBLIC inline
bool 
Mem_space::set_attributes(Address virt, unsigned page_attribs)
{
  Pd_entry *p = _dir->lookup(virt)
    ->pdp()->lookup(virt)
    ->pdir()->lookup(virt);
  Pt_entry *e;

  if (!p->valid() || p->superpage())
    return 0;

  e = p->ptab()->lookup(virt);
  if (!e->valid())
    return 0;

  e->del_attr(Page::MAX_ATTRIBS);
  e->add_attr(page_attribs);
  return true;
}


PRIVATE
void
Mem_space::dir_shutdown()
{
  // free all user page tables we have allocated for this address space
  // except the ones in kernel space which are always shared

  Address address = 0;
  
  // iterate over pml4 table 
  for (unsigned i_pml4 = 0; (i_pml4 < Pml4::Entries) && 
      (address < Kmem::mem_user_max); i_pml4++)
    {
      bool complete = true;
      address = Paging::canonize(address);

      if (!((*_dir)[i_pml4] & Pml4_entry::Valid))
	{
	  address += Config::PML4_SIZE;
	  continue;
	}

      // iterate over pdp table 
      Pdp *_pdp = _dir->lookup(address)->pdp();
      unsigned i_pdp;
      for (i_pdp = 0; (i_pdp < Pdp::Entries) && 
	  (address < Kmem::mem_user_max); i_pdp++)
	{
	  if (!((*_pdp)[i_pdp] & Pdp_entry::Valid))
	    {
	      address += Config::PDP_SIZE;
	      continue;
	    }

	  // iterate over pdir table 
	  Pdir *_pdir = _pdp->lookup(address)->pdir();
	  unsigned i_pdir;
	  for (i_pdir = 0; (i_pdir < Pdir::Entries) && 
	      (address < Kmem::mem_user_max); i_pdir++)
	    {
	      if (    ((*_pdir)[i_pdir] & Pd_entry::Valid)
		  && !((*_pdir)[i_pdir] & (Pd_entry::Superpage | 
		      Pd_entry::global())))
		{
		  Mapped_allocator::allocator()
		    ->free_phys(Config::PAGE_SHIFT, 
			P_ptr<void>((*_pdir)[i_pdir] & Config::PAGE_MASK));

		}
	      address += Config::SUPERPAGE_SIZE;
	    }

	  if (i_pdir == Pdir::Entries)
	    Mapped_allocator::allocator()
	      ->free_phys(Config::PAGE_SHIFT, 
		  P_ptr<void>((*_pdp)[i_pdp] & Config::PAGE_MASK));
	  else
	    complete = false;
	}

      if (i_pdp == Pdp::Entries && complete)
	  Mapped_allocator::allocator()
	    ->free_phys(Config::PAGE_SHIFT, 
		P_ptr<void>((*_dir)[i_pml4] & Config::PAGE_MASK));
    }
  // free IO bitmap (if allocated)
  if (Config::enable_io_protection)
    {
      const Address ports_base = Config::Small_spaces
				   ? Mem_layout::Smas_io_bmap_bak
				   : Mem_layout::Io_bitmap;

      Pd_entry iopde;
      //Pd_entry iopde = *(_dir->lookup(ports_base));

      // do we have an IO bitmap?
      if (iopde.valid())
	{
	  // sanity check
	  assert (!iopde.superpage());

          // free the first half of the IO bitmap
          Pt_entry iopte = *(iopde.ptab()->lookup(ports_base));

          if (iopte.valid())
            Mapped_allocator::allocator()
	      ->free_phys(Config::PAGE_SHIFT, P_ptr <void> (iopte.pfn()));

          // free the second half
          iopte = *(iopde.ptab()->lookup(ports_base + Config::PAGE_SIZE));

          if (iopte.valid())
            Mapped_allocator::allocator()
	      ->free_phys(Config::PAGE_SHIFT, P_ptr <void> (iopte.pfn()));

          // free the page table
          Mapped_allocator::allocator()
	    ->free_phys(Config::PAGE_SHIFT, P_ptr <void> (iopde.ptabfn()));
        }
    }

  // free ldt memory if it was allocated
  free_ldt_memory();

  address = Kmem::mem_user_max;
  // now free kernel page directory
  // iterate over pml4 table 
  for (unsigned i_pml4 = Pml4::virt_to_idx(address)
       ; (i_pml4 < Pml4::Entries) &&  (address > 0)
       ; i_pml4++, address += Config::PML4_SIZE)
    {
      if (!((*_dir)[i_pml4] & Pml4_entry::Valid))
	continue;

      // iterate over pdp table 
      Pdp *_pdp = _dir->lookup(address)->pdp();
      for (unsigned i_pdp = Pdp::virt_to_idx(address)
	   ; (i_pdp < Pdp::Entries) && (address > 0)
	   ; i_pdp++, address += Config::PDP_SIZE)
	{
	  if (!((*_pdp)[i_pdp] & Pdp_entry::Valid))
	    continue;

	  // free pdir table, ptables are shared
	  Mapped_allocator::allocator()
	    ->free_phys(Config::PAGE_SHIFT, 
		P_ptr<void>((*_pdp)[i_pdp] & Config::PAGE_MASK));
	}

      Mapped_allocator::allocator()
	->free_phys(Config::PAGE_SHIFT, 
	    P_ptr<void>((*_dir)[i_pml4] & Config::PAGE_MASK));
    }
}

/** Insert a page-table entry, or upgrade an existing entry with new
    attributes.
    @param phys Physical address (page-aligned).
    @param virt Virtual address for which an entry should be created.
    @param size Size of the page frame -- 4KB or 2MB. 
    @param page_attribs Attributes for the mapping (see
                        Mem_space::Page_attrib).
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
IMPLEMENT
Mem_space::Status
Mem_space::v_insert (Address phys, Address virt, size_t size, 
		 unsigned page_attribs)
{
  // insert page into page table
  
  assert (size == Config::PAGE_SIZE || size == Config::SUPERPAGE_SIZE);
  if (size == Config::SUPERPAGE_SIZE)
    {
      assert (Cpu::have_superpages());
      assert ((virt & ~Config::SUPERPAGE_MASK) == 0);
      assert ((phys & ~Config::SUPERPAGE_MASK) == 0);
    }


  // XXX should modify page table using compare-and-swap

  Unsigned64 attr = Pd_entry::Valid | Pd_entry::Writable
			| Pd_entry::Referenced | Pd_entry::User;
  Pml4_entry *pml4_entry = _dir->lookup(virt);
  Pdp_entry *pdp_entry = pml4_entry->alloc_pdp(Mapped_allocator::allocator(),
					       attr)
    				       ->lookup(virt);
  Pd_entry *p = pdp_entry->alloc_pdir(Mapped_allocator::allocator(), attr)
    			     ->lookup(virt);
  Pt_entry 	*e;

  pml4_entry->add_attr(Pml4_entry::User);
  pdp_entry->add_attr(Pml4_entry::User);
  
  // This 2 MB area is not mapped
  if (EXPECT_TRUE (!p->valid()))
    {
      // check if we can insert a 2MB mapping
      if (size == Config::SUPERPAGE_SIZE)
        {
          *p = phys | Pd_entry::Superpage  | Pd_entry::Valid
		    | page_attribs;

          page_map (phys, virt, size, page_attribs);
	  update_small (virt, false);
          return Insert_ok;
        }
         
      // can't map a superpage -- alloc new page table
      Ptab *new_pt = (Ptab *)Mapped_allocator::allocator()
	->alloc(Config::PAGE_SHIFT);

      if (EXPECT_FALSE (!new_pt))
        return Insert_err_nomem;

      new_pt->clear();

      *p = Kmem::virt_to_phys(new_pt) | Pd_entry::Valid | Pd_entry::Writable
				      | Pd_entry::Referenced | Pd_entry::User;
      update_small (virt, false);
      e = new_pt->lookup(virt);
    }

  // This 2 MB area is covered by a superpage
  else if (EXPECT_TRUE (p->superpage()))
    {
      // want to change physical attributes?
      if (EXPECT_FALSE (size != Config::SUPERPAGE_SIZE 
	    || p->superfn() != phys))
	return Insert_err_exists;

      // same mapping -- check for page-attribute upgrade
      if (EXPECT_FALSE ((p->raw() | page_attribs) == p->raw()))
	// no attrib change
        return Insert_warn_exists;

      // upgrade the whole superpage at once
      p->add_attr(page_attribs);
          
      page_protect (virt, size, p->raw() & Page_all_attribs);
      update_small (virt, false);
      return Insert_warn_attrib_upgrade;
    }

  // This 2 MB area is covered by a pagetable
  else if (EXPECT_FALSE (size == Config::SUPERPAGE_SIZE))
    return Insert_err_exists;

  else
    e = p->ptab()->lookup(virt);

  assert (size == Config::PAGE_SIZE);
  
  // anything mapped already?
  if (EXPECT_FALSE (e->valid()) )
    {
      // different page?
      if (EXPECT_FALSE (e->pfn() != phys) )
        return Insert_err_exists;
      
      // no attrib change?
      if (EXPECT_FALSE ((e->raw() | page_attribs) == e->raw()))
        return Insert_warn_exists;

      // upgrade from read-only to read-write
      e->add_attr(page_attribs);

      page_protect (virt, size, e->raw() & Page_all_attribs);

      return Insert_warn_attrib_upgrade;
    }
  else                  // we don't have mapped anything
    {
      *e = phys | Pt_entry::Valid | page_attribs;

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
                frame (see Mem_space::Page_attrib).
    @param size If not 0, we fill in the size of the page-table slot.  If an
                entry was found (and we return true), this is the size
                of the page frame.  If no entry was found (and we
                return false), this is the size of the free slot.  In
                either case, it is either 4KB or 4MB.
    @return True if an entry was found, false otherwise.
 */
IMPLEMENT
bool
Mem_space::v_lookup (Address virt, Address *phys = 0, Address *size = 0,
	         unsigned *page_attribs = 0)
{
  Pml4_entry *pml4_entry = _dir->lookup(virt);

  if (! pml4_entry->valid())
    {
      if (size) *size = 1UL << 39; // 512 GB
      return false;
    }

  Pdp_entry *pdp_entry = pml4_entry->pdp()->lookup(virt);

  if (! pdp_entry->valid())
    {
      if (size) *size = 1 << 30; // 1 GB
      return false;
    }
  
  Pd_entry *p = pdp_entry->pdir()->lookup(virt);

  if (! p->valid())
    {
      if (size) *size = Config::SUPERPAGE_SIZE;
      return false;
    }

  if (p->superpage())
    {
      if (phys) *phys = p->superfn();
      if (size) *size = Config::SUPERPAGE_SIZE;
      if (page_attribs) *page_attribs = (p->raw() & Page_all_attribs);

      return true;
    }

  const Pt_entry *e = p->ptab()->lookup(virt);

  if (size) *size = Config::PAGE_SIZE;

  if (! e->valid())
    return false;

  if (phys) *phys = e->pfn();
  if (page_attribs) *page_attribs = (e->raw() & Page_all_attribs);

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
IMPLEMENT
unsigned long
Mem_space::v_delete (Address va, unsigned long size, 
		 unsigned long page_attribs = Page_all_attribs)
{
  // delete pages from page tables
  unsigned long ret;
  assert (size == Config::PAGE_SIZE || size == Config::SUPERPAGE_SIZE);
  if (size == Config::SUPERPAGE_SIZE)
    assert ((va & ~Config::SUPERPAGE_MASK) == 0);

  Pml4_entry *pml4_entry = _dir->lookup(va);

  if (! pml4_entry->valid())
    {
      if (Config::conservative)
	kdb_ke("v_delete did not delete anything");
      return 0;
    }
      
  Pdp_entry *pdp_entry = pml4_entry->pdp()->lookup(va);
  if (! pdp_entry->valid())
    {
      if (Config::conservative)
	kdb_ke("v_delete did not delete anything");
      return 0;
    }

  Pd_entry *p = pdp_entry->pdir()->lookup(va);
  Pt_entry *e;
      
  if (! p->valid())
    {
      if (Config::conservative)
	kdb_ke("v_delete did not delete anything");
      return 0;
    }

  assert (!(p->raw() & Pd_entry::global())); // Connot unmap shared pages

  if (EXPECT_FALSE (p->raw() & Pd_entry::Superpage))
    {
      if (size != Config::SUPERPAGE_SIZE)
	{
	  if (Config::conservative)
	    kdb_ke("v_delete did not find superpage");
	  return 0;
	}

      // XXX The downgrade should be atomic with the access extraction
      ret = p->raw() & page_attribs;

      if (! (page_attribs & Page_user_accessible))
	{
	  // downgrade PDE (superpage) rights
	  p->del_attr(page_attribs);
	  page_protect (va, size, p->raw() & Page_all_attribs);
	}
      else
	{
	  // delete PDE (superpage)
	  *p = 0;
	  page_unmap (va, size);
	}
      update_small (va, true);
      return ret;
    } 

  e = p->ptab()->lookup(va);

  if (EXPECT_FALSE (! e->valid()))
    {
      if (Config::conservative)
	kdb_ke("v_delete unmapped page");
      return 0;
    }

  // XXX The downgrade should be atomic with the access extraction
  ret = e->raw() & page_attribs;

  if (! (page_attribs & Page_user_accessible))
    {
      // downgrade PTE rights
      e->del_attr(page_attribs);
      page_protect (va, size, e->raw() & Page_all_attribs);
    }
  else
    {
      // delete PTE
      *e = 0;
      page_unmap (va, size);
    }
  return ret;
}

IMPLEMENT inline
Address
Mem_space::kip_address() const
{
  return (is_sigma0()
	  ? Kmem::virt_to_phys (Kip::k())
	  : (Address)Mem_layout::Kip_auto_map);
}

// --------------------------------------------------------------------
IMPLEMENTATION [!segments,ux]:

PRIVATE inline
void
Mem_space::free_ldt_memory()
{}

// --------------------------------------------------------------------
IMPLEMENTATION [{ux,amd64}-utcb]:

#include "mem_layout.h"
#include "utcb.h"
#include "l4_types.h"
#include "paging.h"

IMPLEMENT inline NEEDS["mem_layout.h", "utcb.h","l4_types.h"]
bool
Mem_space::is_mappable(Address addr, size_t size) 
{
  if ((addr + size) <= Mem_layout::V2_utcb_addr)
    return true;

  if (addr >= Mem_layout::V2_utcb_addr + utcb_size_pages()*Config::PAGE_SIZE)
    return true;

  return false;
}

// --------------------------------------------------------------------
IMPLEMENTATION [{ux,amd64}-!utcb]:

IMPLEMENT inline
bool
Mem_space::is_mappable(Address, size_t)
{
  return true;
}
