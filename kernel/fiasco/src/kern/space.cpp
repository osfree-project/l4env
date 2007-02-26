INTERFACE:

#include "kmem.h"		// for "_unused_*" virtual memory regions
#include "space_context.h"

#include <flux/x86/paging.h>	// for page attributes


/** An address space.
    Class Space extends Space_context with insertion and lookup 
    functions.
 */
class Space : public Space_context
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
  enum Page_attrib {
    Page_no_attribs = 0,
    /// Page is writable.
    Page_writable = INTEL_PTE_WRITE, 
    /// Page is noncacheable.
    Page_noncacheable = INTEL_PTE_NCACHE | INTEL_PTE_WTHRU, 
    /// it's a user page.
    Page_user_accessible = INTEL_PTE_USER, 
    /// A mask which contains all mask bits
    Page_all_attribs = Page_writable | Page_noncacheable | Page_user_accessible
  };

private:
  Space();			// default constructors are undefined
  Space(const Space&);

  inline Address get_virt_port_addr(Address const port) const;

  // store the number of the address space in an unused part of the page dir
  static const unsigned long number_index 
    = (Kmem::_unused1_1_addr >> PDESHIFT) & PDEMASK;
  static const unsigned long chief_index
    = (Kmem::_unused2_1_addr >> PDESHIFT) & PDEMASK;
  static const unsigned long kip_index
    = (Kmem::_unused3_1_addr >> PDESHIFT) & PDEMASK;
};

IMPLEMENTATION:

#include "config.h"
#include "globals.h"
#include "l4_types.h"
#include "kmem_alloc.h"
#include "panic.h"

// state requests/manipulation

inline
Space *
current_space()
{  
  return static_cast<Space*>(Space_context::current());
}

// allocator and deallocator

PUBLIC void *
Space::operator new(size_t)
{
  void *ret;

  check((ret = Kmem_alloc::allocator()->alloc(0)));

  return ret;
}

PUBLIC void 
Space::operator delete(void *block)
{
  Kmem_alloc::allocator()->free(0,block);
}

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
Address
Space::virt_to_phys(Address a) const // pgtble lookup
{
  if (this == sigma0)		// sigma0 doesn't have mapped everything
    return a;

  return Space_context::lookup(a, 0);
}

/** Tests if a task is privileged. This is the case if it has 
    all the IO ports mapped.
    @return true if the task is privileged, false otherwise.
*/    
PUBLIC inline NEEDS["l4_types.h"]
bool Space::is_privileged(void) 
{
  return    ! Config::enable_io_protection 
	 || (get_io_counter() == L4_fpage::IO_PORT_MAX);
}

/** Tests if a task is the sigma0 task.
    @return true if the task is sigma0, false otherwise.
*/
PUBLIC inline NEEDS ["config.h"]
bool Space::is_sigma0(void)
{
  return this == sigma0;
  //return space() == Config::sigma0_taskno;
}

