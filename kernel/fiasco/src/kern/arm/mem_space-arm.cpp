INTERFACE:

#include "kmem.h"		// for "_unused_*" virtual memory regions
#include "paging.h"
#include "types.h"
#include "pagetable.h"
#include "ram_quota.h"
#include "space_index.h"

class Mem_space
{
public:
  typedef Page_table Dir_type;
  typedef Address Phys_addr;

  static char const * const name;

  void make_current();
  static Mem_space *kspace;

  Address lookup( void * ) const;
  void switchin_context();

public:
  /** Return status of v_insert. */
  enum Status {
    Insert_ok = Page_table::E_OK,		///< Mapping was added successfully.
    Insert_err_nomem  = Page_table::E_NOMEM,  ///< Couldn't alloc new page table
    Insert_err_exists = Page_table::E_EXISTS, ///< A mapping already exists at the target addr
    Insert_warn_attrib_upgrade = Page_table::E_UPGRADE,	///< Mapping already existed, attribs upgrade
    Insert_warn_exists,		///< Mapping already existed

  };

  /** Attribute masks for page mappings. */
  enum Page_attrib {
    Page_no_attribs = 0,
    /// Page is writable.
    Page_writable = Mem_page_attr::Write,
    Page_user_accessible = Mem_page_attr::User,
    /// Page is noncacheable.
    Page_noncacheable = Page::NONCACHEABLE,
    Page_cacheable = Page::CACHEABLE,
    /// it's a user page (USER_NO | USER_RO = USER_RW).
    /// A mask which contains all mask bits
    Page_all_attribs = Page_user_accessible | Page_writable | Page_cacheable,
    Page_referenced = 0,
    Page_dirty = 0,
  };

  enum				// Definitions for map_util
  {
    Map_page_size = Config::PAGE_SIZE,
    Map_superpage_size = Config::SUPERPAGE_SIZE,
    Map_max_address = Mem_layout::User_max,
    Has_superpage = 1,
    Whole_space = 32,
    Identity_map = 0,

  };

  
  bool v_lookup(Address virt, Address *phys = 0,
		Address *size = 0, unsigned *page_attribs = 0);

  unsigned long v_delete(Address virt, unsigned long size,
		unsigned long page_attribs = Page_all_attribs);

  Status v_insert(Address phys, Address virt, 
		    size_t size, Mword page_attribs);

  static Mem_space *kernel_space();
  static void kernel_space( Mem_space * );

  void kmem_update (void *addr);
  bool is_mappable (Address, size_t);

  Dir_type *dir() const { return _dir; }

private:
  static inline Mem_space *current_mem_space();
  friend inline Mem_space* current_mem_space();	// Mem_space::current_space

private:
  // DATA
  Ram_quota *_quota;
  Dir_type *_dir;
};

IMPLEMENTATION [arm]:

#include <cassert>
#include <cstring>

#include "atomic.h"
#include "config.h"
#include "globals.h"
#include "kdb_ke.h"
#include "mapped_alloc.h"
#include "l4_types.h"
#include "panic.h"
#include "paging.h"
#include "kmem.h"
#include "mem_unit.h"
#include "utcb.h"

// 
// class Mem_space
// 

char const * const Mem_space::name = "Mem_space";

PUBLIC inline
Ram_quota *
Mem_space::ram_quota() const
{ return _quota; }

PUBLIC inline
bool
Mem_space::valid() const
{ return _dir; }

// Mapping utilities

/**
 * Get size of UTCB area
 */
PRIVATE inline NEEDS ["utcb.h"]
Mword
Mem_space::utcb_size_pages()
{
  return (L4_uid::threads_per_task()*sizeof(Utcb) + (Config::PAGE_SIZE -1))
          / Config::PAGE_SIZE;
}

IMPLEMENT inline NEEDS[Mem_space::utcb_size_pages]
bool
Mem_space::is_mappable(Address addr, size_t size)
{
  if ((addr + size) <= Mem_layout::V2_utcb_addr)
    return true;

  if (addr >= Mem_layout::V2_utcb_addr + utcb_size_pages()*Config::PAGE_SIZE)
    return true;

  return false;
}

PUBLIC static inline NEEDS["mem_unit.h"]
void
Mem_space::tlb_flush()
{
  Mem_unit::tlb_flush();
}

PUBLIC inline 
bool
Mem_space::need_tlb_flush()
{
  return false;
}

Mem_space *Mem_space::kspace;

PUBLIC inline
void
Mem_space::enable_reverse_lookup()
{
  // Store reverse pointer to Space in page directory
  assert(((unsigned long)this & 0x03) == 0);
  Pte pte = _dir->walk((void*)Mem_layout::Space_index, 
      Config::SUPERPAGE_SIZE, false, 0 /*does never allocate*/);

  pte.set_invalid((unsigned long)this, false);
}

IMPLEMENT inline
Mem_space *Mem_space::current_mem_space()
{
  Pte pte = Page_table::current()->walk((void*)Mem_layout::Space_index, 
      Config::SUPERPAGE_SIZE, false, 0 /*does never allocate*/);
  return reinterpret_cast<Mem_space*>(pte.raw());
}

inline NEEDS[Mem_space::current_mem_space]
Mem_space *
current_mem_space()
{  
  return Mem_space::current_mem_space();
}


PRIVATE inline
Page_table *Mem_space::current_pdir()
{
  return Page_table::current();
}

IMPLEMENT inline
Address Mem_space::lookup( void *a ) const
{
  Pte pte = _dir->walk(a, 0, false, 0 /*does never allocate*/);
  if (EXPECT_FALSE(!pte.valid()))
    return ~0UL;

  return pte.phys(a);
}


IMPLEMENT inline NEEDS ["kmem.h", Mem_space::c_asid] 
void Mem_space::switchin_context()
{

  // never switch to kernel space (context of the idle thread)
  if (this == kernel_space())
    return;

  _dir->invalidate((void*)Kmem::ipc_window(0), Config::SUPERPAGE_SIZE * 4,
      c_asid());

  if (Page_table::current() != dir())
    make_current();

}

IMPLEMENT inline
Mem_space *Mem_space::kernel_space()
{
  return kspace;
}

IMPLEMENT inline
void Mem_space::kernel_space( Mem_space *_k_space )
{
  kspace = _k_space;
}


#if 0
/** Update this address space with an entry from another address space.
    Use this function when a part of another address space needs to be
    temporarily visible in the current address space (i.e., for long
    IPC).  It copies a page-directory entry from the source address
    space, making visible a 4-MB window in this address space.
    @param addr virtual address where temporary should be established
    @param from source address space
    @param from_addr virtual address in source address space.
    @pre addr and from_addr are 4MB-aligned
 */
PUBLIC inline bool 
Mem_space::update(Address addr, const Mem_space *from, 
	      Address from_addr)
{
  if (from->lookup(from_addr) == 0xffffffff)
    return false;

  _dir[(addr >> PDESHIFT) & PDEMASK] 
    = from->dir()[(from_addr >> PDESHIFT) & PDEMASK];
  return true;
}

#endif

// routines


/**
 * Simple page-table lookup.
 *
 * @param virt Virtual address.  This address does not need to be page-aligned.
 * @return Physical address corresponding to a.
 */
PUBLIC inline NEEDS ["paging.h"]
Address
Mem_space::virt_to_phys (Address virt) const
{
  return (Address)Mem_space::lookup((void *)virt);
}

PUBLIC inline NEEDS [Mem_space::virt_to_phys]
Address
Mem_space::pmem_to_phys (Address virt) const
{
  return virt_to_phys(virt);
}

/** Simple page-table lookup.  This method is similar to Mem_space's 
    lookup().  The difference is that this version handles 
    Sigma0's address space with a special case: For Sigma0, we do not 
    actually consult the page table -- it is meaningless because we
    create new mappings for Sigma0 transparently; instead, we return the
    logically-correct result of physical address == virtual address.
    @param a Virtual address.  This address does not need to be page-aligned.
    @return Physical address corresponding to a.
 */

PUBLIC inline NEEDS["globals.h"]
Address Mem_space::virt_to_phys_s0(void *a) const // pgtble lookup
{
  if (this == sigma0_space)	// sigma0 doesn't have mapped everything
    return (Address)a/*+0xc0000000*/;

  return (Address)Mem_space::lookup(a);
}

IMPLEMENT
bool Mem_space::v_lookup(Address virt, Address *phys,
		     Address *size, unsigned *page_attribs)
{
  Pte p = _dir->walk( (void*)virt, 0, false,0);
  
  if (size) *size = p.size();
  if (page_attribs) *page_attribs = p.attr().get_abstract();
  if (phys) *phys = p.phys((void*)virt);
  return p.valid();
}

IMPLEMENT
unsigned long
Mem_space::v_delete(Address virt, unsigned long size,
    unsigned long page_attribs)
{
  bool flush = Page_table::current() == _dir;
  Pte pte = _dir->walk((void*)virt, 0, false, ram_quota());
  if (EXPECT_FALSE(!pte.valid()))
    return 0;

  if (EXPECT_FALSE(pte.size() != size))
    {
      kdb_ke("v_del: size mismatch\n");
      return 0;
    }

  Mem_unit::flush_vcache((void*)(virt & ~(pte.size()-1)), 
      (void*)((virt & ~(pte.size()-1)) + pte.size()));

  Mem_page_attr a = pte.attr();
  unsigned long abs_a = a.get_abstract();

  if (!(page_attribs & Page_user_accessible))
    {
      a.set_ap(abs_a & ~page_attribs);
      pte.attr(a, flush);
    }
  else
    pte.set_invalid(0, flush);

  Mem_unit::tlb_flush((void*)virt, c_asid());

  return abs_a & page_attribs;
}

IMPLEMENT
Mem_space::Status Mem_space::v_insert(Address phys, Address virt, 
			      size_t size, Mword page_attribs)
{
  bool flush = Page_table::current() == _dir;
  Pte pte = _dir->walk((void*)virt, size, flush, ram_quota());
  if (pte.valid())
    {
      if (EXPECT_FALSE(pte.size() != size || pte.phys() != phys))
	return Insert_err_exists;
      if (pte.attr().get_abstract() == page_attribs)
	return Insert_warn_exists;
    }
  else
    {
      Mem_page_attr a(Page::Local_page);
      a.set_abstract(page_attribs);
      pte.set(phys, size, a, flush);
      return Insert_ok;
    }

  Mem_page_attr a = pte.attr();
  a.set_abstract(a.get_abstract() | page_attribs);
  pte.set(phys, size, a, flush);
  Mem_unit::tlb_flush((void*)virt, c_asid());
  return Insert_warn_attrib_upgrade;
}

PUBLIC inline NEEDS[Mem_space::is_sigma0, "config.h"]
bool
Mem_space::v_fabricate (unsigned /*task_id*/, Address address, 
		    Address* phys, Address* size, unsigned* attribs = 0)
{
  if (is_sigma0())
    {
      // special-cased because we don't do ptab lookup for sigma0
      *phys = address & Config::SUPERPAGE_MASK;
      *size = Config::SUPERPAGE_SIZE;
      if (attribs)
	*attribs = Page_writable | Page_user_accessible | Page_cacheable;
      return true;
    }

  return false;
}

PUBLIC inline
bool 
Mem_space::set_attributes(Address virt, unsigned page_attribs)
{
  Pte p = _dir->walk( (void*)virt, 0, false,0);
  if (!p.valid()) 
    return false;

  Mem_page_attr a = p.attr();
  a.set_ap(page_attribs);
  p.attr(a, true);
  return true;
}

IMPLEMENT inline NEEDS[Mem_space::c_asid]
void Mem_space::kmem_update (void *addr)
{
  // copy current shared kernel page directory
  _dir->copy_in(addr, kernel_space()->_dir, 
	  addr, Config::SUPERPAGE_SIZE, c_asid());

}


/** 
 * Tests if a task is the sigma0 task.
 * @return true if the task is sigma0, false otherwise.
 */
PUBLIC inline 
bool Mem_space::is_sigma0(void)
{
  return this == sigma0_space;
}

PUBLIC
Mem_space::~Mem_space()
{
  if (_dir)
    {
      _dir->free_page_tables(0, (void*)Mem_layout::User_max);
      delete _dir;
      ram_quota()->free(sizeof(Page_table));
    }
}

PUBLIC
void
Mem_space::reset_dirty()
{
  _dir = 0;
}

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
PUBLIC
Mem_space::Mem_space(Ram_quota *q)
  : _quota(q), _dir(0)
{
  asid(~0UL);

  if (EXPECT_FALSE(!ram_quota()->alloc(sizeof(Page_table))))
      return;

  _dir = new Page_table();
  assert(_dir);

  // copy current shared kernel page directory
  _dir->copy_in((void*)Mem_layout::User_max, 
      kernel_space()->_dir, 
      (void*)Mem_layout::User_max, 
      Mem_layout::Kernel_max - Mem_layout::User_max);

  enable_reverse_lookup ();
}

PUBLIC
Mem_space::Mem_space (Dir_type* pdir)
  : _dir (pdir)
{
  asid(0);
  enable_reverse_lookup ();
}


//----------------------------------------------------------------------------
IMPLEMENTATION [armv5]:

PRIVATE inline 
void
Mem_space::asid(unsigned long)
{}

PUBLIC inline 
unsigned long
Mem_space::c_asid() const
{ return 0; }

IMPLEMENT inline
void Mem_space::make_current()
{
  _dir->activate();
}


//----------------------------------------------------------------------------
INTERFACE [armv6]:

EXTENSION class Mem_space
{
private:
  unsigned long _asid;

  static unsigned char _next_free_asid;
  static Mem_space *_active_asids[256];
};


//----------------------------------------------------------------------------
IMPLEMENTATION [armv6]:


unsigned char Mem_space::_next_free_asid;
Mem_space *Mem_space::_active_asids[256];

PRIVATE inline 
void
Mem_space::asid(unsigned long a)
{ _asid = a; }

PUBLIC inline 
unsigned long
Mem_space::c_asid() const
{ return _asid; }

PRIVATE inline static
unsigned long
Mem_space::next_asid()
{ 
  unsigned long ret = _next_free_asid++;
  return ret; 
}

PRIVATE inline NEEDS[Mem_space::next_asid]
unsigned long
Mem_space::asid()
{
  if (EXPECT_FALSE(_asid == ~0UL))
    {
      // FIFO ASID replacement strategy
      unsigned char new_asid = next_asid();
      Mem_space **bad_guy = &_active_asids[new_asid];
      while (*bad_guy)
	{
	  // need ASID replacement
	  if (*bad_guy == current_mem_space())
	    {
	      // do not replace the ASID of the current space
	      new_asid = next_asid();
	      bad_guy = &_active_asids[new_asid];
	      continue;
	    }

	  //LOG_MSG_3VAL(current(), "ASIDr", new_asid, (Mword)*bad_guy, (Mword)this);
	  Mem_unit::tlb_flush((*bad_guy)->_asid);
	  (*bad_guy)->_asid = ~0UL;

	  break;
	}

      *bad_guy = this;
      _asid = new_asid;
    }

  //LOG_MSG_3VAL(current(), "ASID", (Mword)this, _asid, (Mword)__builtin_return_address(0));
  return _asid;
};

IMPLEMENT inline NEEDS[Mem_space::asid]
void Mem_space::make_current()
{
  _dir->activate(asid());
}

