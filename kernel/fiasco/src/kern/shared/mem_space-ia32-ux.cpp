INTERFACE [ia32,ux]:

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
  void remote_update (Address, Mem_space const *,Address, size_t);

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

IMPLEMENTATION [ia32,ux]:

#include <cstring>
#include <cstdio>
#include "cpu.h"
#include "kdb_ke.h"
#include "l4_types.h"
#include "mem_layout.h"
#include "paging.h"
#include "std_macros.h"
#include "utcb.h"

IMPLEMENT inline NEEDS["mem_layout.h", "utcb.h","l4_types.h"]
bool
Mem_space::is_mappable(Address addr, size_t size) 
{
  if (size == Config::SUPERPAGE_SIZE && ! Cpu::have_superpages())
    return false;

  if ((addr + size) <= Mem_layout::V2_utcb_addr)
    return true;

  if (addr >= Mem_layout::V2_utcb_addr + utcb_size_pages()*Config::PAGE_SIZE)
    return true;

  return false;
}


PUBLIC inline
bool 
Mem_space::set_attributes(Address virt, unsigned page_attribs)
{
  Pd_entry *p = _dir->lookup(virt);
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


PRIVATE inline
void
Mem_space::enable_reverse_lookup()
{
  // Store reverse pointer to Mem_space in page directory
  *(_dir->lookup(Mem_layout::Space_index)) =
    reinterpret_cast<Unsigned32>(this);
}

/**
 * Destructor.  Deletes the address space and unregisters it from
 * Space_index.
 */
PRIVATE
void
Mem_space::dir_shutdown()
{
  // free all page tables we have allocated for this address space
  // except the ones in kernel space which are always shared

  for (unsigned i=0; i<(Kmem::mem_user_max >> Config::SUPERPAGE_SHIFT); i++)
    if (    ((*_dir)[i] & Pd_entry::Valid)
	&& !((*_dir)[i] & (Pd_entry::Superpage | Pd_entry::global())))
      Mapped_allocator::allocator()
	->q_free_phys(_quota, Config::PAGE_SHIFT, 
	    P_ptr<void>((*_dir)[i] & Config::PAGE_MASK));

  // free IO bitmap (if allocated) -- this should have already
  // happened in ~Io_space
  if (Config::enable_io_protection)
    {
      assert (! _dir->lookup(Config::Small_spaces
			     ? Mem_layout::Smas_io_bmap_bak
			     : Mem_layout::Io_bitmap)
	            ->valid());
    }

  // free ldt memory if it was allocated
  free_ldt_memory();
}

/** Insert a page-table entry, or upgrade an existing entry with new
    attributes.
    @param phys Physical address (page-aligned).
    @param virt Virtual address for which an entry should be created.
    @param size Size of the page frame -- 4KB or 4MB.
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

  // XXX should modify page table using compare-and-swap
  
  assert (size == Config::PAGE_SIZE || size == Config::SUPERPAGE_SIZE);
  if (size == Config::SUPERPAGE_SIZE)
    {
      assert (Cpu::have_superpages());
      assert ((virt & ~Config::SUPERPAGE_MASK) == 0);
      assert ((phys & ~Config::SUPERPAGE_MASK) == 0);
    }

  Pd_entry *p = _dir->lookup(virt);
  Pt_entry *e;
  
  // This 4 MB area is not mapped
  if (EXPECT_TRUE (!p->valid()))
    {
      // check if we can insert a 4MB mapping
      if (size == Config::SUPERPAGE_SIZE)
	{
          *p = phys | Pd_entry::Superpage  | Pd_entry::Valid
		    | page_attribs;

          page_map (phys, virt, size, page_attribs);
	  update_small (virt, false);
          return Insert_ok;
        }
       
      Ptab *new_pt;
      // can't map a superpage -- alloc new page table
      if (EXPECT_FALSE(!(new_pt = (Ptab *)Mapped_allocator::allocator()
	         ->q_alloc(_quota, Config::PAGE_SHIFT))))
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
      if (EXPECT_FALSE (size != Config::SUPERPAGE_SIZE 
			|| p->superfn() != phys))
	return Insert_err_exists;

      // same mapping -- check for page-attribute upgrade
      if (EXPECT_FALSE ((p->raw() | page_attribs) == p->raw()))
	// no attrib change
        return Insert_warn_exists;

      // upgrade the superpage
      p->add_attr(page_attribs);
          
      page_protect (virt, size, p->raw() & Page_all_attribs);
      update_small (virt, false);
      return Insert_warn_attrib_upgrade;
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
  const Pd_entry *p = _dir->lookup(virt);

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
    @return Combined (bit-ORed) page attributes that were removed.  In
            case of errors, ~Page_all_attribs is additionally bit-ORed in.
 */
IMPLEMENT
unsigned long
Mem_space::v_delete (Address virt, unsigned long size, 
		 unsigned long page_attribs = Page_all_attribs)
{
  unsigned ret;

  // delete pages from page tables
  assert (size == Config::PAGE_SIZE || size == Config::SUPERPAGE_SIZE);
  if (size == Config::SUPERPAGE_SIZE)
    {
      assert (Cpu::have_superpages());
      assert ((virt & ~Config::SUPERPAGE_MASK) == 0);
    }

  Pd_entry *p = _dir->lookup(virt);
  Pt_entry *e;
      
  if (EXPECT_FALSE (! p->valid()))
    {
      if (Config::conservative)
	kdb_ke("v_delete did not find anything");

      return 0;
    }

  assert (! (p->raw() & Pd_entry::global())); // Cannot unmap shared ptables

  if (EXPECT_FALSE (p->raw() & Pd_entry::Superpage))
    {
      assert (size == Config::SUPERPAGE_SIZE);
//       if (size != Config::SUPERPAGE_SIZE)
// 	{
// 	  if (Config::conservative)
// 	    kdb_ke("v_delete did not find superpage");
// 	  return 0;
// 	}

      // XXX The downgrade should be atomic with the access extraction
      ret = p->raw() & page_attribs;

      if (! (page_attribs & Page_user_accessible))
	{
	  // downgrade PDE (superpage) rights
	  p->del_attr(page_attribs);
	  page_protect (virt, size, p->raw() & Page_all_attribs);
	}
      else
	{
	  // delete PDE (superpage)
	  *p = 0;
	  page_unmap (virt, size);
	}
      
      update_small (virt, true);
      return ret;
    } 

  assert (size == Config::PAGE_SIZE);

  e = p->ptab()->lookup(virt);

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
      page_protect (virt, size, e->raw() & Page_all_attribs);
    }
  else
    {
      // delete PTE
      *e = 0;
      page_unmap (virt, size);
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
INTERFACE [(ia32 || amd64) && !smas]:

EXTENSION class Mem_space
{
private:
  void *_ldt_addr;
  Mword _ldt_size;
};

// --------------------------------------------------------------------
IMPLEMENTATION [(ia32 || amd64) && !smas]:

PUBLIC inline
void
Mem_space::ldt_addr(void *addr)
{ _ldt_addr = addr; }

PUBLIC inline
Address
Mem_space::ldt_addr() const
{ return (Address)_ldt_addr; }

PUBLIC inline
void
Mem_space::ldt_size(Mword size)
{ _ldt_size = size; }

PUBLIC inline
Mword
Mem_space::ldt_size() const
{ return _ldt_size; }


