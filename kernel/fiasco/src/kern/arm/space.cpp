INTERFACE:

#include "kmem.h"		// for "_unused_*" virtual memory regions
#include "paging.h"
#include "space_index.h"
#include "space_context.h"

class Space : public Space_context
{
public:
  /** Return status of v_insert. */
  enum status_t {
    Insert_ok = E_OK,		///< Mapping was added successfully.
    Insert_err_nomem  = E_NOMEM,  ///< Couldn't alloc new page table
    Insert_err_exists = E_EXISTS, ///< A mapping already exists at the target addr
    Insert_warn_attrib_upgrade = E_UPGRADE,	///< Mapping already existed, attribs upgrade
    Insert_warn_exists,		///< Mapping already existed

  };

  /** Attribute masks for page mappings. */
  enum page_attrib_t {
    Page_no_attribs = 0,
    /// Page is writable.
    Page_writable = Page::USER_NO, 
    /// Page is noncacheable.
    Page_noncacheable = Page::NONCACHEABLE | Page::WRITETHROUGH, 
    /// it's a user page (USER_NO | USER_RO = USER_RW).
    Page_user_accessible = Page::USER_RO, 
    /// A mask which contains all mask bits
    Page_all_attribs = Page_writable | Page_noncacheable | Page_user_accessible
  };

  
  bool v_lookup(Address virt, Address *phys = 0,
		size_t *size = 0, Mword *page_attribs = 0);

  bool v_delete(Address virt, size_t size,
		Mword page_attribs = 0);

  status_t v_insert(Address phys, Address virt, 
		    size_t size, Mword page_attribs);

private:
  Space();			// default constructors are undefined
  Space(const Space&);

  enum {
    number_index = 0xeff00000, 
    chief_index  = 0xefe00000,
  };

};

IMPLEMENTATION:

#include <cassert>
#include <cstring>

#include "atomic.h"
#include "config.h"
#include "globals.h"
#include "kdb_ke.h"
#include "kmem_alloc.h"
#include "l4_types.h"
#include "panic.h"

// state requests/manipulation

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
#warning NEED dir_init
  //Kmem::dir_init(_dir);		// copy current shared kernel page directory

  // scribble task/chief numbers into an unused part of the page directory
  insert_invalid((void*)number_index, Config::SUPERPAGE_SIZE, new_number << 8);	      
  insert_invalid((void*)chief_index, Config::SUPERPAGE_SIZE,
		 Space_index(new_number).chief() << 8);

  Space_index::add(this, new_number);		// register in task table
}


inline
Space *current_space()
{  
  return nonull_static_cast<Space*>(Space_context::current());
}

/** Task number.
    @return Number of the task to which this Space instance 
             belongs
 */
PUBLIC inline 
Space_index  Space::space() const		// returns task number
{
  return Space_index(lookup_invalid((void*)number_index) >> 8);
}

/** Task number of this task's chief.
    @return Task number of this task's chief
 */
PUBLIC inline 
Space_index Space::chief() const		// returns chief number
{
  return Space_index(lookup_invalid((void*)chief_index) >> 8);
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
Space::update(Address addr, const Space_context *from, 
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

/** Simple page-table lookup.  This method is similar to Space_context's 
    lookup().  The difference is that this version handles 
    Sigma0's address space with a special case: For Sigma0, we do not 
    actually consult the page table -- it is meaningless because we
    create new mappings for Sigma0 transparently; instead, we return the
    logically-correct result of physical address == virtual address.
    @param a Virtual address.  This address does not need to be page-aligned.
    @return Physical address corresponding to a.
 */

PUBLIC inline NEEDS["globals.h"]
Address Space::virt_to_phys(void *a) const // pgtble lookup
{
  if (this == sigma0)		// sigma0 doesn't have mapped everything
    return (Address)a;

  return (Address)Space_context::lookup(a);
}


IMPLEMENT
bool Space::v_lookup(Address virt, Address *phys,
		     size_t *size, Mword *page_attribs)
{
  P_ptr<void> p = Page_table::lookup( (void*)virt, size, page_attribs );
  if( phys ) *phys = p.get_unsigned();
  return ! p.is_null();
}

IMPLEMENT
bool Space::v_delete(Address /*virt*/, size_t /*size*/,
		     Mword /*page_attribs*/)
{
# warning NOT IMPLEMENTED
  return true;
}

IMPLEMENT
Space::status_t Space::v_insert(Address phys, Address virt, 
				size_t size, Mword page_attribs)
{
  size_t s;
  Page::Attribs a;
  P_ptr<void> p = Page_table::lookup( (void*)virt, &s, &a );
  if(p.is_null())
    return (status_t)insert( P_ptr<void>(phys), (void *)virt, size, page_attribs );
  else
    {
      if(p.get_unsigned() != phys || s != size)
	return Insert_err_exists;
      if(page_attribs == a)
	return Insert_warn_exists;
      
#warning Page_table::change not yet implemented
      //      change((void*)virt, page_attribs);
      return Insert_warn_attrib_upgrade;
    }
}
