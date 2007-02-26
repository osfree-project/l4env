INTERFACE[ia32,ux]:

#include "space_index.h"

INTERFACE:

#include "paging.h"		// for page attributes

/**
 * An address space.
 *
 * Class Space extends Space_context with insertion and lookup functions.
 */
class Space
{
public:
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
      /// Page is noncacheable.
      Page_noncacheable = Pt_entry::Noncacheable | Pt_entry::Write_through, 
      /// it's a user page.
      Page_user_accessible = Pt_entry::User,
      /// A mask which contains all mask bits
      Page_all_attribs = Page_writable | Page_noncacheable |
			 Page_user_accessible,
    };

  /// Is this task a privileged one?
  bool is_privileged (void);

  static Space *id_lookup(Task_num id);

public:
  static Space *current();
  void          make_current();
  void          switchin_context();

protected:
  void switchin_lipc_kip_pointer();

  // DATA (be careful here!):  just a page directory
  Pdir                    _dir;
};

INTERFACE [utcb]:

EXTENSION class Space
{
public:
  /// The number of pages occupied by the UTCB area.
  Mword utcb_size_pages();
};

INTERFACE [!arm]:

EXTENSION class Space
{
private:
  Space();			// default constructors are undefined
  Space(const Space&);
};


IMPLEMENTATION:

#include "config.h"
#include "globals.h"
#include "l4_types.h"
#include "mapped_alloc.h"
#include "panic.h"

// state requests/manipulation
PUBLIC inline NEEDS ["paging.h", "config.h"]
int
Space::mapped (Address addr, int need_writable) const
{
  Pd_entry p = _dir.entry(addr);

  if (!p.valid())
    return 0;

  if (p.superpage())
    return !need_writable || p.writable();

  Pt_entry e = *(p.ptab()->lookup(addr));

  return e.valid() && (!need_writable || e.writable());
}

/**
 * Simple page-table lookup.
 *
 * @param virt Virtual address.  This address does not need to be page-aligned.
 * @return Physical address corresponding to a.
 */
PUBLIC inline NEEDS ["paging.h"]
Address
Space::virt_to_phys (Address virt) const
{
  return dir()->virt_to_phys(virt);
}

PUBLIC inline
const Pdir*
Space::dir () const
{
  return &_dir;
}

IMPLEMENTATION [ia32,ux]:

/**
 * Task number.
 * @return Number of the task to which this Space instance belongs.
 */
PUBLIC inline 
Space_index
Space::id() const
{
  return Space_index (_dir.entry(Mem_layout::Space_index).raw() >> 8);
}

/**
 * Chief Number.
 * @return Task number of this task's chief.
 */
PUBLIC inline 
Space_index 
Space::chief() const
{
  return Space_index (_dir.entry(Mem_layout::Chief_index).raw() >> 8);
}

IMPLEMENTATION[!lipc]:

IMPLEMENT inline
void
Space::switchin_lipc_kip_pointer()
{}

//-----------------------------------------------------------------------------
IMPLEMENTATION[{ia32,ux}-v2-lipc]:

#include "config.h"
#include "types.h"
#include "paging.h"
#include "globals.h"
#include "mem_layout.h"

/** Every Task can his LIPC-kip page somewhere, so we need to store this
    kip location for every task. An unused page dir slot is used for this.
    On every task switch load the new kip_location from the _dir[kip_index]
    of the new task and write it in the utcb pointer page.
 */
IMPLEMENT inline NEEDS["types.h", "globals.h", "mem_layout.h", "paging.h"]
void
Space::switchin_lipc_kip_pointer()
{
  user_kip_location = _dir.entry(Mem_layout::Kip_index).raw();
}

/** Sets the new LIPC kip location for this task
    @param addr   new LIPC-KIP location
 */
PUBLIC inline NEEDS["mem_layout.h", "paging.h"]
void
Space::set_lipc_kip_pointer(Address addr)
{
  _dir[(Mem_layout::Kip_index >> Pd_entry::Shift) & Pd_entry::Mask] = addr;
}

/** Returns the current LIPC-KIP location for this task
    @return LIPC-KIP location
 */
PUBLIC inline NEEDS["mem_layout.h", "paging.h"]
Address
Space::get_lipc_kip_pointer()
{
  return _dir.entry(Mem_layout::Kip_index).raw();
}

//-----------------------------------------------------------------------------
IMPLEMENTATION[{ia32,ux}-v2-utcb-!lipc]:

PUBLIC inline
void
Space::set_lipc_kip_pointer(Address /*addr*/)
{}


IMPLEMENTATION:

inline
Space *
current_space()
{  
  return Space::current();
}

// allocator and deallocator

PUBLIC void *
Space::operator new (size_t size)
{
  void *ret;

  (void)size;
  assert (size == Config::PAGE_SIZE);

  check((ret = Mapped_allocator::allocator()->alloc(Config::PAGE_SHIFT)));

  return ret;
}

PUBLIC void 
Space::operator delete (void *block)
{
  Mapped_allocator::allocator()->free(Config::PAGE_SHIFT, block);
}

// routines

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
Space::virt_to_phys_s0 (void *a) const // pgtble lookup
{
  if (is_sigma0())		// sigma0 doesn't have mapped everything
    return (Address)a;

  return dir()->virt_to_phys((Address)a);
}

/**
 * Tests if a task is the sigma0 task.
 * @return true if the task is sigma0, false otherwise.
 */
PUBLIC inline NEEDS ["globals.h","config.h"]
bool Space::is_sigma0 (void) const
{
  return this == sigma0_space;
}

/// Lookup space belonging to a task number.
IMPLEMENT inline
Space *
Space::id_lookup (Task_num id)
{
  return Space_index (id).lookup();
}


//---------------------------------------------------------------------------
IMPLEMENTATION [!io]:

PUBLIC inline
unsigned
Space::get_io_counter (void)
{ return 0; }

PUBLIC inline
bool
Space::io_lookup (Address)
{ return false; }

// --------------------------------------------------------------------
IMPLEMENTATION [utcb]:

#include "mem_layout.h"
#include "utcb.h"
#include "l4_types.h"
#include "paging.h"

/**
 * Get size of UTCB area
 */
IMPLEMENT inline NEEDS ["utcb.h"]
Mword
Space::utcb_size_pages()
{
  return (L4_uid::threads_per_task()*sizeof(Utcb) + (Config::PAGE_SIZE -1))
          / Config::PAGE_SIZE;
}

//---------------------------------------------------------------------------
IMPLEMENTATION [!arm]:

IMPLEMENT inline NEEDS ["l4_types.h"]
bool Space::is_privileged (void) 
{
  // A task is privileged if it has all the IO ports mapped.
  return (!Config::enable_io_protection 
	  || (get_io_counter() == L4_fpage::Io_port_max));
}
