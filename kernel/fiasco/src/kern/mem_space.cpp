INTERFACE:

#include "paging.h"		// for page attributes
#include "mem_layout.h"
#include "ram_quota.h"

//class Space;

/**
 * An address space.
 *
 * Insertion and lookup functions.
 */
class Mem_space
{
public:
  typedef Address Phys_addr;

  static char const * const name;

  /** Return status of v_insert. */
  enum Status {
    Insert_ok = 0,		///< Mapping was added successfully.
    Insert_warn_exists,		///< Mapping already existed
    Insert_warn_attrib_upgrade,	///< Mapping already existed, attribs upgrade
    Insert_err_nomem,		///< Couldn't alloc new page table
    Insert_err_exists		///< A mapping already exists at the target addr
  };

  /** Attribute masks for page mappings. */
  enum Page_attrib 
    {
      Page_no_attribs = 0,
      /// Page is writable.
      Page_writable = Pt_entry::Writable, 
      Page_cacheable = 0, 
      /// Page is noncacheable.
      Page_noncacheable = Pt_entry::Noncacheable | Pt_entry::Write_through, 
      /// it's a user page.
      Page_user_accessible = Pt_entry::User,
      /// Page has been referenced
      Page_referenced = Pt_entry::Referenced,
      /// Page is dirty
      Page_dirty = Pt_entry::Dirty,
      /// A mask which contains all mask bits
      Page_all_attribs = Page_writable | Page_noncacheable |
			 Page_user_accessible | Page_referenced | Page_dirty,
    };

  // Mapping utilities

  enum				// Definitions for map_util
    {
      Has_superpage = 1,
      Map_page_size = Config::PAGE_SIZE,
      Map_superpage_size = Config::SUPERPAGE_SIZE,
      Map_max_address = Mem_layout::User_max,
      Whole_space = L4_fpage::Whole_space,
      Identity_map = 0,
    };

public:
  // Each architecture must provide these members:
  void switchin_context();

  Status v_insert (Address phys, Address virt, size_t size,
      unsigned page_attribs);

  bool v_lookup (Address virt, Address *phys = 0, Address *size = 0,
      unsigned *page_attribs = 0);


  unsigned long v_delete(Address virt, unsigned long size, 
      unsigned long page_attribs = Page_all_attribs);


private:
  Ram_quota *_quota;
  // Each architecture must provide these members

  // Page-table ops
  // We'd like to declare current_pdir here, but Dir_type isn't defined yet.
  // static inline Dir_type *current_pdir();
  inline void make_current();

  // Space reverse lookup
  static inline Mem_space *current_mem_space();
  friend inline Mem_space* current_mem_space();	// Mem_space::current_space 

  // Mem_space();
  Mem_space(const Mem_space &);	// undefined copy constructor

  static Mem_space *_current asm ("CURRENT_MEM_SPACE");
};

INTERFACE [utcb]:

EXTENSION class Mem_space
{
public:
  /// The number of pages occupied by the UTCB area.
  Mword utcb_size_pages();
};

IMPLEMENTATION:

// 
// class Mem_space
// 

#include "config.h"
#include "globals.h"
#include "l4_types.h"
#include "mapped_alloc.h"
#include "mem_unit.h"
#include "panic.h"


char const * const Mem_space::name = "Mem_space";
Mem_space *Mem_space::_current;

PUBLIC
Mem_space::Mem_space (Ram_quota *q)
  : _quota(q), _dir (0)
{
  void *b;
  if (EXPECT_FALSE(! (b = Mapped_allocator::allocator()
	  ->q_alloc(_quota, Config::PAGE_SHIFT))))
    return;

  _dir = static_cast<Dir_type*>(b);
  _dir->clear();	// initialize to zero
  Kmem::dir_init(_dir);	// copy current shared kernel page directory
  enable_reverse_lookup();
}

PUBLIC
Mem_space::Mem_space (Dir_type* pdir)
  : _dir (pdir)
{
  _current = this;
  enable_reverse_lookup();
}

IMPLEMENT inline
Mem_space *
Mem_space::current_mem_space()
{
  return _current;
}

/**
 * Destructor.  Deletes the address space and unregisters it from
 * Space_index.
 */
PUBLIC
Mem_space::~Mem_space()
{
  if (_dir)
    {
      dir_shutdown();

      Mapped_allocator::allocator()->q_free(_quota, Config::PAGE_SHIFT, _dir);
    }
}

PUBLIC inline
bool
Mem_space::valid() const
{ return _dir; }

PUBLIC inline
Ram_quota *
Mem_space::ram_quota() const
{ return _quota; }


/// Avoid deallocation of page table upon Mem_space destruction.
PUBLIC
void
Mem_space::reset_dirty ()
{
  _dir = 0;			
}

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
  return dir()->virt_to_phys(virt);
}

/**
 * Simple page-table lookup.
 *
 * This method is similar to Space_context's virt_to_phys().
 * The difference is that this version handles Sigma0's
 * address space with a special case:  For Sigma0, we do not 
 * actually consult the page table -- it is meaningless because we
 * create new mappings for Sigma0 transparently; instead, we return the
 * logically-correct result of physical address == virtual address.
 *
 * @param a Virtual address.  This address does not need to be page-aligned.
 * @return Physical address corresponding to a.
 */
PUBLIC inline NEEDS ["globals.h"]
Address
Mem_space::virt_to_phys_s0 (void *a) const // pgtble lookup
{
  if (is_sigma0())		// sigma0 doesn't have mapped everything
    return (Address)a;

  return dir()->virt_to_phys((Address)a);
}

PUBLIC inline
Mem_space::Dir_type*
Mem_space::dir ()
{
  return _dir;
}

PUBLIC inline
const Mem_space::Dir_type*
Mem_space::dir () const
{
  return _dir;
}

inline NEEDS[Mem_space::current_mem_space]
Mem_space *
current_mem_space()
{  
  return Mem_space::current_mem_space();
}

// routines

/**
 * Tests if a task is the sigma0 task.
 * @return true if the task is sigma0, false otherwise.
 */
PUBLIC inline NEEDS ["globals.h","config.h"]
bool Mem_space::is_sigma0 () const
{
  return this == sigma0_space;
}

// Mapping utilities

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

PUBLIC static inline NEEDS["mem_unit.h"]
void
Mem_space::tlb_flush()
{
  Mem_unit::tlb_flush();
}

PUBLIC inline NEEDS["config.h", Mem_space::current_pdir]
bool
Mem_space::need_tlb_flush()
{
  // Not so rare case that we delete mappings in our host space.
  // With small adress spaces there will be no flush on switch
  // anymore.
  if (Config::Small_spaces && dir() == current_pdir())
    return true;
  return false;
}

//-----------------------------------------------------------------------------
IMPLEMENTATION[ia32,ux]:

// state requests/manipulation
PUBLIC inline NEEDS ["paging.h", "config.h"]
int
Mem_space::mapped (Address addr, int need_writable) const
{
  Pd_entry p = _dir->entry(addr);

  if (!p.valid())
    return 0;

  if (p.superpage())
    return !need_writable || p.writable();

  Pt_entry e = *(p.ptab()->lookup(addr));

  return e.valid() && (!need_writable || e.writable());
}

//-----------------------------------------------------------------------------
IMPLEMENTATION[amd64]:

// state requests/manipulation
PUBLIC inline NEEDS ["paging.h", "config.h"]
int
Mem_space::mapped (Address addr, int need_writable) const
{
  Pml4_entry pml4_entry = _dir->entry(addr);

  if (!pml4_entry.valid())
    return 0;

  Pdp_entry pdp_entry = *(pml4_entry.pdp()->lookup(addr));

  if (!pdp_entry.valid())
    return 0;

  Pd_entry pd_entry = *(pdp_entry.pdir()->lookup(addr));
  
  if (!pd_entry.valid())
    return 0;

  if (pd_entry.superpage())
    return !need_writable || pd_entry.writable();

  Pt_entry pt_entry = *(pd_entry.ptab()->lookup(addr));

  return pt_entry.valid() && (!need_writable || pt_entry.writable());
}


//---------------------------------------------------------------------------
IMPLEMENTATION [!io]:

PUBLIC inline
Mword
Mem_space::get_io_counter (void)
{ return 0; }

PUBLIC inline
bool
Mem_space::io_lookup (Address)
{ return false; }

// --------------------------------------------------------------------
IMPLEMENTATION [utcb]:

#include "utcb.h"
#include "l4_types.h"
#include "paging.h"

/**
 * Get size of UTCB area
 */
IMPLEMENT inline NEEDS ["utcb.h"]
Mword
Mem_space::utcb_size_pages()
{
  return (L4_uid::threads_per_task()*sizeof(Utcb) + (Config::PAGE_SIZE -1))
          / Config::PAGE_SIZE;
}

