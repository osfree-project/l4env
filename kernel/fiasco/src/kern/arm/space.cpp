INTERFACE:

#include "kmem.h"		// for "_unused_*" virtual memory regions
#include "paging.h"
#include "types.h"
#include "pagetable.h"
#include "space_index.h"

class Space : public Page_table
{
public:
  static Space *current();
  void make_current();
  static Space *kspace;

  using Page_table::lookup;
  Address lookup( void * ) const;
  void switchin_context();

public:
  /** Return status of v_insert. */
  enum Status {
    Insert_ok = E_OK,		///< Mapping was added successfully.
    Insert_err_nomem  = E_NOMEM,  ///< Couldn't alloc new page table
    Insert_err_exists = E_EXISTS, ///< A mapping already exists at the target addr
    Insert_warn_attrib_upgrade = E_UPGRADE,	///< Mapping already existed, attribs upgrade
    Insert_warn_exists,		///< Mapping already existed

  };

  /** Attribute masks for page mappings. */
  enum Page_attrib {
    Page_no_attribs = 0,
    /// Page is writable.
    Page_writable = Page::USER_NO, 
    /// Page is noncacheable.
    Page_noncacheable = Page::NONCACHEABLE, 
    /// it's a user page (USER_NO | USER_RO = USER_RW).
    Page_user_accessible = Page::USER_RO, 
    /// A mask which contains all mask bits
    Page_all_attribs = Page_writable | Page_noncacheable | Page_user_accessible
  };
  
  bool v_lookup(Address virt, Address *phys = 0,
		Address *size = 0, unsigned *page_attribs = 0);

  bool v_delete(Address virt, size_t size,
		Mword page_attribs = 0);

  Status v_insert(Address phys, Address virt, 
		    size_t size, Mword page_attribs);

  static Space *kernel_space();
  static void kernel_space( Space * );

  void kmem_update (void *addr);
  bool is_mappable (Address, size_t);
  static Space *id_lookup(Task_num id);

#if 0
private:
  Space();			// default constructors are undefined
  Space(const Space&);
#endif
  enum {
    number_index = 0xeff00000, 
    chief_index  = 0xefe00000,
  };

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
#include "kmem.h"

Space *Space::kspace;

PUBLIC static inline
Space *Space::kernel_space_context()
{
  return kspace;
}

PUBLIC static inline
void Space::kernel_space_context(Space *v)
{
  kspace = v;
}


IMPLEMENT inline
Space *Space::current()
{
  return nonull_static_cast<Space*>(Page_table::current());
}

IMPLEMENT inline
void Space::make_current()
{
  Page_table::activate();
}

IMPLEMENT inline
Address Space::lookup( void *a ) const
{
  P_ptr<void> ph = Page_table::lookup( a, 0, 0 );
  return ph.get_unsigned();
}


IMPLEMENT inline NEEDS ["kmem.h"] 
void Space::switchin_context()
{

  // never switch to kernel space (context of the idle thread)
  if (this == kernel_space_context())
    return;
  
  invalidate((void*)Kmem::ipc_window(0), Config::SUPERPAGE_SIZE * 4, true);
  
  if (Space::current() != this)
    make_current();

}

IMPLEMENT inline
Space *Space::kernel_space()
{
  return static_cast<Space *>(kernel_space_context());
}

IMPLEMENT inline
void Space::kernel_space( Space *_k_space )
{
  kernel_space_context((Space *)_k_space);
}

// state requests/manipulation


inline
Space *current_space()
{  
  return nonull_static_cast<Space*>(Space::current());
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
Space::update(Address addr, const Space *from, 
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

/** Simple page-table lookup.  This method is similar to Space's 
    lookup().  The difference is that this version handles 
    Sigma0's address space with a special case: For Sigma0, we do not 
    actually consult the page table -- it is meaningless because we
    create new mappings for Sigma0 transparently; instead, we return the
    logically-correct result of physical address == virtual address.
    @param a Virtual address.  This address does not need to be page-aligned.
    @return Physical address corresponding to a.
 */

PUBLIC inline NEEDS["globals.h"]
Address Space::virt_to_phys_s0(void *a) const // pgtble lookup
{
  if (this == sigma0_space)	// sigma0 doesn't have mapped everything
    return (Address)a/*+0xc0000000*/;

  return (Address)Space::lookup(a);
}


IMPLEMENT
bool Space::v_lookup(Address virt, Address *phys,
		     Address *size, unsigned *page_attribs)
{
  P_ptr<void> p = Page_table::lookup( (void*)virt, size, page_attribs );
  if( phys ) *phys = p.get_unsigned();
  return ! p.is_null();
}

IMPLEMENT
bool Space::v_delete(Address virt, size_t size,
		     Mword page_attribs)
{
  Address s;
  Page::Attribs a;
  P_ptr<void> p = Page_table::lookup( (void*)virt, &s, &a );
  if (p.is_null())
    return true;

  if (s != size)
    return false;
  
  if (page_attribs)
    Page_table::change( (void*)virt, page_attribs);
  else
    Page_table::remove( (void*)virt );
  
  return true;
}

IMPLEMENT
Space::Status Space::v_insert(Address phys, Address virt, 
			      size_t size, Mword page_attribs)
{
  Address s;
  Page::Attribs a;
  P_ptr<void> p = Page_table::lookup( (void*)virt, &s, &a );
  if(p.is_null())
    return (Status)insert( P_ptr<void>(phys), (void*)virt, size, page_attribs);
  else
    {
      if(p.get_unsigned() != phys || s != size)
	return Insert_err_exists;
      if(page_attribs == a)
	return Insert_warn_exists;

      change((void*)virt, page_attribs);

      return Insert_warn_attrib_upgrade;
    }
}

IMPLEMENT inline
void Space::kmem_update (void *addr)
{
  // copy current shared kernel page directory
  copy_in(addr, nonull_static_cast<Page_table*>(kernel_space()), 
	  addr, Config::SUPERPAGE_SIZE, true);

}


/** 
 * Tests if a task is the sigma0 task.
 * @return true if the task is sigma0, false otherwise.
 */
PUBLIC inline 
bool Space::is_sigma0(void)
{
  return this == sigma0_space;
}

PUBLIC
Space::~Space()
{
  free_page_tables(0, (void*)Mem_layout::User_max);

  Space_index::del (id(), chief()); // deregister from task table
}


IMPLEMENTATION [arm-x0]:

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
  // copy current shared kernel page directory
  copy_in((void*)Mem_layout::User_max, 
	  nonull_static_cast<Page_table*>(kernel_space()), 
	  (void*)Mem_layout::User_max, 
	  Mem_layout::Kernel_max - Mem_layout::User_max);
  
  // scribble task/chief numbers into an unused part of the page directory
  insert_invalid((void*)number_index, Config::SUPERPAGE_SIZE, new_number << 8);	      
  insert_invalid((void*)chief_index, Config::SUPERPAGE_SIZE,
		 Space_index(new_number).chief() << 8);

  Space_index::add(this, new_number);		// register in task table
}

/** 
 * Task number.
 * @return Number of the task to which this Space instance
 *         belongs
 */
PUBLIC inline 
Space_index Space::id() const		// returns task number
{
  return Space_index(lookup_invalid((void*)number_index) >> 8);
}

/**
 * Task number of this task's chief.
 * @return Task number of this task's chief
 */
PUBLIC inline 
Space_index Space::chief() const		// returns chief number
{
  return Space_index(lookup_invalid((void*)chief_index) >> 8);
}

IMPLEMENT inline
bool
Space::is_mappable(Address, size_t)
{
  return true;
}

IMPLEMENT inline
Space *
Space::id_lookup (Task_num id)
{
  return Space_index (id).lookup();
}
