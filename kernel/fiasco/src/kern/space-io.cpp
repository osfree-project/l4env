IMPLEMENTATION[io]:

#include <cassert>
#include <cstring>

#include "config.h"
#include "l4_types.h"
#include "panic.h"

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
  Pd_entry *p = _dir + io_counter_pd_index;
  if (! (*p & INTEL_PDE_VALID))
    return 0;			// nothing mapped -> no ports

  Unsigned32 *counter = 
    static_cast<Unsigned32 *>(Kmem::phys_to_virt(*p & Config::PAGE_MASK))
	  + io_counter_pt_index;
  
  return *counter >> 8;
}


/** Add something the the IO counter.
    @param incr number to add
    @pre 2nd level page table for IO bitmap is present
*/

PUBLIC void
Space::addto_io_counter(int incr)
{
  Pd_entry *p = _dir + io_counter_pd_index;
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
Space::io_lookup(Address port_number)
{
  assert(port_number < L4_fpage::IO_PORT_MAX);

  // be careful, do not cause page faults here
  // there might be nothing mapped in the IO bitmap

  Address port_addr = get_phys_port_addr(port_number);

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
Space::Status Space::io_insert(Address port_number)
{
  assert(port_number < L4_fpage::IO_PORT_MAX);

  // be careful, do not cause page faults here
  // there might be nothing mapped in the IO bitmap

  Address port_addr = get_phys_port_addr(port_number);

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

      Status status = 
	v_insert( Kmem::virt_to_phys(page), 
		  get_virt_port_addr(port_number) & Config::PAGE_MASK,
		  Config::PAGE_SIZE,
		  Page_no_attribs );
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
void Space::io_delete(Address port_number)
{
  assert(port_number < L4_fpage::IO_PORT_MAX);

  // be careful, do not cause page faults here
  // there might be nothing mapped in the IO bitmap

  Address port_addr = get_phys_port_addr(port_number);

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

IMPLEMENT INLINE
Address
Space::get_virt_port_addr(Address const port) const
{
  return Kmem::io_bitmap + (port >> 3);
}

INLINE
Address
Space::get_phys_port_addr(Address const port) const
{
  // favour lookup over virt_to_phys because we do
  // access the address
  return lookup(get_virt_port_addr(port), 0);
}


INLINE 
Unsigned8 get_port_bit(Address const port)
{
  return 1 << (port & 7);
}
