INTERFACE [ia32,ux]:

EXTENSION class Space
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
  void remote_update (const Address, const Space *,
                      const Address, size_t);

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

private:
  /// Remove space from space index.
  void remove_from_space_index();
};

IMPLEMENTATION [ia32,ux]:

#include <cstring>
#include <cstdio>
#include "cpu.h"
#include "kdb_ke.h"
#include "std_macros.h"

/**
 * Destructor.  Deletes the address space and unregisters it from
 * Space_index.
 */
PUBLIC
Space::~Space()
{
  // free all page tables we have allocated for this address space
  // except the ones in kernel space which are always shared

  for (unsigned i=0; i<(Kmem::mem_user_max >> Config::SUPERPAGE_SHIFT); i++)
    if (    (_dir[i] & Pd_entry::Valid)
	&& !(_dir[i] & (Pd_entry::Superpage | Pd_entry::global())))
      Mapped_allocator::allocator()
	->free_phys(Config::PAGE_SHIFT, 
	            P_ptr<void>(_dir[i] & Config::PAGE_MASK));

  // free IO bitmap (if allocated)
  if (Config::enable_io_protection)
    {
      const Address ports_base = Config::Small_spaces
				   ? Mem_layout::Smas_io_bmap_bak
				   : Mem_layout::Io_bitmap;

      Pd_entry iopde = *(_dir.lookup(ports_base));

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

  // deregister from task table
  remove_from_space_index();
}

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
Space::Status
Space::v_insert (Address phys, Address virt, size_t size, 
		 unsigned page_attribs)
{
  // insert page into page table

  // XXX should modify page table using compare-and-swap
 
  Pd_entry *p = _dir.lookup(virt);
  Pt_entry *e;
  
  // This 4 MB area is not mapped
  if (EXPECT_TRUE (!p->valid()))
    {
      // check if we can insert a 4MB mapping
      if (size == Config::SUPERPAGE_SIZE
          && Cpu::have_superpages()
          && ((virt & ~Config::SUPERPAGE_MASK) == 0) 
	  && ((phys & ~Config::SUPERPAGE_MASK) == 0) )
        {
          *p = phys | Pd_entry::Superpage  | Pd_entry::Valid
		    | Pd_entry::Referenced | Pd_entry::Dirty
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

  // This 4 MB area is covered by a superpage
  else if (EXPECT_TRUE (p->superpage()))
    {
      // want to change physical attributes?
      if (EXPECT_FALSE ((p->superfn() | (virt & ~Config::SUPERPAGE_MASK))
			!= phys) )
        return Insert_err_exists;

      // same mapping -- check for page-attribute upgrade
      if (EXPECT_FALSE ((p->raw() | page_attribs) == p->raw()))
	// no attrib change
        return Insert_warn_exists;

      // change attributes?
      if (size == Config::SUPERPAGE_SIZE)
        {
          // upgrade the whole superpage at once
          p->add_attr(page_attribs);
          
          page_protect (virt, size, p->raw() & Page_all_attribs);
	  update_small (virt, false);
          return Insert_warn_attrib_upgrade;
        }

      // we need to split the superpage mapping because
      // it's only partly upgraded

      Ptab *new_pt = split_pgtable(p);

      if (EXPECT_FALSE (! new_pt) )
        return Insert_err_nomem;
      
      *p = Kmem::virt_to_phys(new_pt) | Pd_entry::Valid | Pd_entry::Writable
				      | Pd_entry::Referenced | Pd_entry::User;
      update_small (virt, false);
      e = new_pt->lookup(virt);
    }

  // This 4 MB area is covered by a pagetable
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
      *e = phys | Pt_entry::Dirty | Pt_entry::Valid | Pt_entry::Referenced
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
bool
Space::v_lookup (Address virt, Address *phys = 0, Address *size = 0,
	         unsigned *page_attribs = 0)
{
  const Pd_entry *p = _dir.lookup(virt);

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
PUBLIC
bool Space::v_delete (Address virt, Address size,
		      unsigned page_attribs = 0)
{
  // delete pages from page tables
  
  for (Address va = virt; va < virt + size; va += Config::PAGE_SIZE)
    {
      Pd_entry *p = _dir.lookup(va);
      Pt_entry *e;
      
      if (! p->valid())
        {
          // no page dir entry -- warp to next page dir entry
          va = ((va + Config::SUPERPAGE_SIZE) & Config::SUPERPAGE_MASK)
	     - Config::PAGE_SIZE;

          if (Config::conservative)
            kdb_ke("v_delete unmapped pgtable");

          continue;
        }

      if (p->raw() & (Pd_entry::Superpage | Pd_entry::global()))
        {
          // oops, a superpage or a shared ptable; see if we can unmap
          // it at once, otherwise split it

          if (((va & ~Config::SUPERPAGE_MASK) == 0) // superpage boundary
              && va + Config::SUPERPAGE_SIZE <= virt + size)
            {
              if (page_attribs)		
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
              va += Config::SUPERPAGE_SIZE - Config::PAGE_SIZE;
              continue;
            }

          // need to unshare/split

          Ptab *new_pt = split_pgtable(p);
          if (!new_pt)
            return false;
          
          *p = Kmem::virt_to_phys(new_pt)
	     | Pd_entry::Valid | Pd_entry::Writable  
	     | Pd_entry::Referenced | Pd_entry::User;
	  update_small (va, true);
          e = new_pt->lookup(va);
        } 

      else
        e = p->ptab()->lookup(va);

      if (Config::conservative && !e->valid())
        kdb_ke("v_delete unmapped page");

      if (page_attribs)
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
    }

  return true;
}

static
Ptab*
split_pgtable (Pd_entry *p)
{
  // alloc new page table
  Ptab *new_pt = static_cast<Ptab *>(Mapped_allocator::allocator()
      ->alloc(Config::PAGE_SHIFT));

  if (!new_pt)
    return 0;

  if (p->superpage())
    for (unsigned i = 0; i < 1024; i++)
      (*new_pt)[i] = (p->superfn() + (i << Config::PAGE_SHIFT))
		   | Pt_entry::Dirty | Pt_entry::Valid | Pt_entry::Referenced
		   | (p->raw() & Space::Page_all_attribs);

  else
    Cpu::memcpy_mwords (new_pt, p, Config::PAGE_SIZE / sizeof(Mword));

  return new_pt;
}

IMPLEMENT inline
Address
Space::kip_address() const
{
  return (is_sigma0()
	  ? Kmem::virt_to_phys (Kip::k())
	  : (Address)Mem_layout::Kip_auto_map);
}

IMPLEMENT inline
void
Space::remove_from_space_index()
{
  Space_index::del (id(), chief());
}

// --------------------------------------------------------------------
IMPLEMENTATION [!segments,ux]:

PRIVATE inline
void
Space::free_ldt_memory()
{}

// --------------------------------------------------------------------
IMPLEMENTATION [{ia32,ux}-utcb]:

#include "mem_layout.h"
#include "utcb.h"
#include "l4_types.h"
#include "paging.h"

IMPLEMENT inline NEEDS["mem_layout.h", "utcb.h","l4_types.h"]
bool
Space::is_mappable(Address addr, size_t size) 
{
  if ((addr + size) <= Mem_layout::V2_utcb_addr)
    return true;

  if (addr >= Mem_layout::V2_utcb_addr + utcb_size_pages()*Config::PAGE_SIZE)
    return true;

  return false;
}

// --------------------------------------------------------------------
IMPLEMENTATION [{ia32,ux}-!utcb]:

IMPLEMENT inline
bool
Space::is_mappable(Address, size_t)
{
  return true;
}
