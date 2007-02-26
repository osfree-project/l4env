IMPLEMENTATION [ia32-io,ux-io]:

#include <cassert>
#include <cstring>

#include "atomic.h"
#include "config.h"
#include "l4_types.h"
#include "panic.h"

//
// IO lookup / insert / delete / counting
//

/** return the IO counter that is scribbled in the second level 
 *  page table for the IO bitmap.
 *  @return number of IO ports mapped / 0 if not mapped
 */
PUBLIC
unsigned
Space::get_io_counter(void) const
{
  const Address virt = Config::Small_spaces ? Mem_layout::Smas_io_cnt_bak
					    : Mem_layout::Io_counter;
  Pd_entry p = _dir.entry(virt);
  return p.valid() ? p.ptab()->entry(virt).raw() >> 1 
		   : 0;
}


/** Add something the the IO counter.
    @param incr number to add
    @pre 2nd level page table for IO bitmap is present
*/
PUBLIC inline
void
Space::addto_io_counter(int incr)
{
  const Address virt = Config::Small_spaces ? Mem_layout::Smas_io_cnt_bak
					    : Mem_layout::Io_counter;
  Pt_entry *e = _dir.lookup(virt)->ptab()->lookup(virt);

  // insert something _NOT_ INTEL_PTE_VALID
  atomic_add ((Mword*)e, incr << 1);
}



/** Lookup one IO port in the IO space.
    @param port_number port address to lookup; 
    @return true if mapped
     false if not 
 */
PUBLIC
bool
Space::io_lookup(Address port_number)
{
  assert(port_number < L4_fpage::Io_port_max);

  // be careful, do not cause page faults here
  // there might be nothing mapped in the IO bitmap

  Address port_addr = get_phys_port_addr(port_number);

  if(port_addr == 0xffffffff)
    return false;		// no bitmap -> no ports

  // so there is memory mapped in the IO bitmap
  char * port = static_cast<char *>(Kmem::phys_to_virt(port_addr));
  
  // bit == 1 disables the port
  // bit == 0 enables the port
  return !(*port & get_port_bit(port_number));
}


/** Enable one IO port in the IO space.
    This function is called in the context of the IPC sender!
    @param port_number address of the port
    @return Insert_warn_exists if some ports were mapped in that IO page
       Insert_err_nomem if memory allocation failed
       Insert_ok if otherwise insertion succeeded
 */
PUBLIC 
Space::Status
Space::io_insert(Address port_number)
{
  assert(port_number < L4_fpage::Io_port_max);

  // if SMAS is active use the shadow IO bitmap since this space context is
  // not active and we switch the the IO bitmap at every context switch
  const Address ports_base = Config::Small_spaces
				   ? Mem_layout::Smas_io_bmap_bak
				   : Mem_layout::Io_bitmap;

  Address port_virt = ports_base + (port_number >> 3);
  Address port_phys = virt_to_phys (port_virt);

  if (port_phys == 0xffffffff)
    {
      // nothing mapped! Get a page and map it in the IO bitmap
      void *page = Mapped_allocator::allocator()->alloc(Config::PAGE_SHIFT);
      if (!page)
	return Insert_err_nomem;

      // clear all IO ports
      // bit == 1 disables the port
      // bit == 0 enables the port
      memset(page, 0xff, Config::PAGE_SIZE);

      Status status = 
	v_insert (Mem_layout::pmem_to_phys(page),
		  port_virt & Config::PAGE_MASK,
		  Config::PAGE_SIZE, Page_writable);

      if (status == Insert_err_nomem)
	{
	  Mapped_allocator::allocator()->free(Config::PAGE_SHIFT,page);
	  return Insert_err_nomem;
	}

      // we've been carful, so insertion should have succeeded
      assert(status == Insert_ok); 

      port_phys = virt_to_phys (port_virt);
      assert(port_phys != 0xffffffff);
    }

  // so there is memory mapped in the IO bitmap -- write the bits now
  Unsigned8 *port = static_cast<Unsigned8 *> (Kmem::phys_to_virt(port_phys));

  if (*port & get_port_bit(port_number)) // port disabled?
    {
      *port &= ~ get_port_bit(port_number);
      addto_io_counter(1);
      return Insert_ok;
    }

  // already enabled
  return Insert_warn_exists;
}


/** Disable one IO port in the IO space.
    @param port_number port to disable
 */
PUBLIC 
void
Space::io_delete(Address port_number)
{
  assert(port_number < L4_fpage::Io_port_max);

  // be careful, do not cause page faults here
  // there might be nothing mapped in the IO bitmap

  Address port_addr = get_phys_port_addr(port_number);

  if (port_addr == 0xffffffff)
    // nothing mapped -> nothing to delete
    return;

  // so there is memory mapped in the IO bitmap -> disable the ports
  char * port = static_cast<char *> (Kmem::phys_to_virt(port_addr));

  // bit == 1 disables the port
  // bit == 0 enables the port
  if(!(*port & get_port_bit(port_number)))    // port enabled ??
    {
      *port |= get_port_bit(port_number);
      addto_io_counter(-1);
    }
}

INLINE NEEDS["config.h"]
Address
Space::get_phys_port_addr(Address const port_number) const
{
  const Address base = Config::Small_spaces ? Mem_layout::Smas_io_bmap_bak
					    : Mem_layout::Io_bitmap;
  return virt_to_phys(base + (port_number >> 3));
}

INLINE 
Unsigned8
Space::get_port_bit(Address const port_number) const
{
  return 1 << (port_number & 7);
}
