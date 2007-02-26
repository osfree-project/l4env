INTERFACE:

#include "kmem.h"		// for "_unused_*" virtual memory regions
#include "space_index.h"
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
  enum status_t {
    Insert_ok = 0,		///< Mapping was added successfully.
    Insert_warn_exists,		///< Mapping already existed
    Insert_warn_attrib_upgrade,	///< Mapping already existed, attribs upgrade
    Insert_err_nomem,		///< Couldn't alloc new page table
    Insert_err_exists		///< A mapping already exists at the target addr
  };

  /** Attribute masks for page mappings. */
  enum page_attrib_t {
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

  // store the number of the address space in an unused part of the page dir
  static const unsigned long number_index 
    = (reinterpret_cast<unsigned long>(&_unused1_1) >> PDESHIFT) & PDEMASK;
  static const unsigned long chief_index
    = (reinterpret_cast<unsigned long>(&_unused2_1) >> PDESHIFT) & PDEMASK;
  // store IO bit counter somewhere in the IO bitmap page table (if present)
  static const unsigned long io_counter_pd_index
    = (reinterpret_cast<unsigned long>(&_unused4_io_1) >> PDESHIFT) & PDEMASK;
  static const unsigned long io_counter_pt_index
    = (reinterpret_cast<unsigned long>(&_unused4_io_1) >> PTESHIFT) & PTEMASK;
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

inline
Space *
current_space()
{  
  return static_cast<Space*>(Space_context::current());
}

/** Task number.
    @return Number of the task to which this Space instance 
             belongs
 */
PUBLIC inline Space_index 
Space::space() const		// returns task number
{
  return Space_index(_dir[number_index] >> 8);
}

/** Task number of this task's chief.
    @return Task number of this task's chief
 */
PUBLIC inline Space_index 
Space::chief() const		// returns chief number
{
  return Space_index(_dir[chief_index] >> 8);
}

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
Space::update(vm_offset_t addr, const Space_context *from, 
		vm_offset_t from_addr)
{
  if (from->lookup(from_addr) == 0xffffffff)
    return false;

  _dir[(addr >> PDESHIFT) & PDEMASK] 
    = from->dir()[(from_addr >> PDESHIFT) & PDEMASK];
  return true;
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
vm_offset_t
Space::virt_to_phys(vm_offset_t a) const // pgtble lookup
{
  if (this == sigma0)		// sigma0 doesn't have mapped everything
    return a;

  return Space_context::lookup(a);
}

//
// IO lookup / insert / delete / counting
//

/** return the IO counter that is scribbled in the second level 
    page table for the IO bitmap.
    @return number of IO ports mapped (hopefully)
*/    

PUBLIC unsigned
Space::get_io_counter(void)
{
  pd_entry_t *p = _dir + io_counter_pd_index;
  if (! (*p & INTEL_PDE_VALID))
    return 0;			// nothing mapped -> no ports

  Unsigned32 *counter = 
    static_cast<Unsigned32 *>(Kmem::phys_to_virt(*p & Config::PAGE_MASK))
	  + io_counter_pt_index;
  
  return *counter >> 8;
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


/** Add something the the IO counter.
    @param incr number to add
    @pre 2nd level page table for IO bitmap is present
*/

PUBLIC void
Space::addto_io_counter(int incr)
{
  pd_entry_t *p = _dir + io_counter_pd_index;
  assert(*p & INTEL_PDE_VALID);

  Unsigned32 * counter = 
    static_cast<Unsigned32 *>(Kmem::phys_to_virt(*p & Config::PAGE_MASK))
	  + io_counter_pt_index;
  
  // insert something _NOT_ INTEL_PTE_VALID
  *counter = ((*counter >> 8) + incr) << 8;
}



/** Lookup one IO port in the IO space.
    @param port_number port address to lookup; 
    @return true if mapped
     false if not 
 */
PUBLIC bool
Space::io_lookup(vm_offset_t port_number)
{
  assert(port_number < L4_fpage::IO_PORT_MAX);

  // be careful, do not cause page faults here
  // there might be nothing mapped in the IO bitmap

  vm_offset_t port_addr = get_phys_port_addr(port_number);

  // printf("io_lookup : %0x\n", port_addr);

  if(port_addr == 0xffffffff)
    return false;		// no bitmap -> no ports

  // so there is memory mapped in the IO bitmap
  char * port = 
    static_cast<char *>(Kmem::phys_to_virt(port_addr));
  
  // bit == 1 disables the port
  // bit == 0 enables the port
  return !(*port & get_port_bit(port_number));
}


/** Enable one IO port in the IO space 
    @param port_number address of the port
    @return Insert_warn_exists if some ports were mapped in that IO page
       Insert_err_nomem if memory allocation failed
       Insert_ok if otherwise insertion succeeded
 */
PUBLIC 
Space::status_t Space::io_insert(vm_offset_t port_number)
{
  assert(port_number < L4_fpage::IO_PORT_MAX);

  // be careful, do not cause page faults here
  // there might be nothing mapped in the IO bitmap

  vm_offset_t port_addr = get_phys_port_addr(port_number);

  if( port_addr == 0xffffffff )
    {
      // nothing mapped! Get a page and map it in the IO bitmap

      void * page = Kmem_alloc::allocator()->alloc(0);
      if( !page )
	return Insert_err_nomem;

      // clear all IO ports
      // bit == 1 disables the port
      // bit == 0 enables the port
      memset(page, 0xff, Config::PAGE_SIZE);

      status_t status = 
	v_insert( Kmem::virt_to_phys(page), 
		  get_virt_port_addr(port_number) & Config::PAGE_MASK,
		  Config::PAGE_SIZE,
		  Page_no_attribs
		  );
      // kdb_ke("HHH");

      if(status == Insert_err_nomem)
	{
	  Kmem_alloc::allocator()->free(0,page);
	  return Insert_err_nomem;
	}

      // we've been carful, so insertion should have succeeded
      assert(status == Insert_ok); 

      port_addr = get_phys_port_addr(port_number);
      assert(port_addr != 0xffffffff);
    }

  // so there is memory mapped in the IO bitmap
  // write the bits now

  Unsigned8 *port = static_cast<Unsigned8 *> (Kmem::phys_to_virt(port_addr));

  if(*port & get_port_bit(port_number))	// port disabled ? 
    {
      *port &= ~ get_port_bit(port_number);

      addto_io_counter(1);

      return Insert_ok;
    }
  else
    {				// already enabled
      return Insert_warn_exists;
    }
}


/** Disable one IO port in the IO space.
    @param port_number port to disable
 */
PUBLIC 
void Space::io_delete(vm_offset_t port_number)
{
  assert(port_number < L4_fpage::IO_PORT_MAX);

  // be careful, do not cause page faults here
  // there might be nothing mapped in the IO bitmap

  vm_offset_t port_addr = get_phys_port_addr(port_number);

  if( port_addr == 0xffffffff )
    {
      // nothing mapped -> nothing to delete
      return;
    }

  // so there is memory mapped in the IO bitmap
  // disable the ports

  char * port = static_cast<char *> (Kmem::phys_to_virt(port_addr));

  // bit == 1 disables the port
  // bit == 0 enables the port
  if(!(*port & get_port_bit(port_number)))    // port enabled ??
    {
      *port |= get_port_bit(port_number);
      addto_io_counter(-1);
    }

  return;
}


//
// IO helpers
//

INLINE
vm_offset_t
Space::get_virt_port_addr(vm_offset_t const port) const
{
  return Kmem::io_bitmap + (port >> 3);
}

INLINE
vm_offset_t
Space::get_phys_port_addr(vm_offset_t const port) const
{
  // favour lookoup over virt_to_phys because we do
  // access the address
  return lookup(get_virt_port_addr(port));
}


INLINE 
Unsigned8 get_port_bit(vm_offset_t const port)
{
  return 1 << (port & 7);
}
