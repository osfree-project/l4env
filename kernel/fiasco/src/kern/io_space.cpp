INTERFACE [ia32-io,ux-io]:

#include "types.h"
#include "mem_space.h"

class Mem_space;

/** Wrapper class for io_{map,unmap}.  This class serves as an adapter
    for map<Io_space> to Mem_space.
 */
class Io_space
{
  friend class Jdb_iomap;

public:
  static char const * const name;
  typedef Address Phys_addr;

  enum {
    Has_superpage = 1,
    Map_page_size = 1,
    Map_superpage_size = 0x10000,
    Map_max_address = 0x10000,
    Whole_space = L4_fpage::Whole_io_space,
    Identity_map = 1,
  };

  // We'd rather like to use a "using Mem_space::Status" declaration here,
  // but that wouldn't make the enum values accessible as
  // Io_space::Insert_ok and so on.
  enum Status {
    Insert_ok = 0,		///< Mapping was added successfully.
    Insert_warn_exists,		///< Mapping already existed
    Insert_warn_attrib_upgrade,	///< Mapping already existed, attribs upgrade
    Insert_err_nomem,		///< Couldn't alloc new page table
    Insert_err_exists		///< A mapping already exists at the target addr
  };

  enum Page_attribs
  {
    Page_writable = Mem_space::Page_writable,
    Page_user_accessible = Mem_space::Page_user_accessible,
    Page_all_attribs = Page_writable | Page_user_accessible
  };

private:
  // DATA
  Mword   _io_counter;
  Mem_space* _mem_space;
};

IMPLEMENTATION [ia32-io,ux-io]:

#include <cassert>
#include <cstring>

#include "atomic.h"
#include "config.h"
#include "l4_types.h"
#include "mapped_alloc.h"
#include "panic.h"
#include "paging.h"
  
char const * const Io_space::name = "Io_space";

PUBLIC inline
Io_space::Io_space ()
  : _io_counter (0),
    _mem_space (0)
{}

PUBLIC inline NEEDS["config.h", "mapped_alloc.h"]
Io_space::~Io_space ()
{
  if (! _mem_space || ! _mem_space->dir())
    return;

  const Address ports_base = Config::Small_spaces
    ? Mem_layout::Smas_io_bmap_bak
    : Mem_layout::Io_bitmap;
  
  Pd_entry *iopde = _mem_space->dir()->lookup(ports_base);
  
  // do we have an IO bitmap?
  if (iopde->valid())
    {
      // sanity check
      assert (!iopde->superpage());
      
      // free the first half of the IO bitmap
      Pt_entry iopte = *(iopde->ptab()->lookup(ports_base));
      
      if (iopte.valid())
	{
	  Mapped_allocator::allocator()
	    ->free_phys(Config::PAGE_SHIFT, P_ptr <void> (iopte.pfn()));
	  _mem_space->ram_quota()->free(Config::PAGE_SIZE);
	}
      
      // free the second half
      iopte = *(iopde->ptab()->lookup(ports_base + Config::PAGE_SIZE));
      
      if (iopte.valid())
	{
	  Mapped_allocator::allocator()
	    ->free_phys(Config::PAGE_SHIFT, P_ptr <void> (iopte.pfn()));
	  _mem_space->ram_quota()->free(Config::PAGE_SIZE);
	}
      
      // free the page table
      Mapped_allocator::allocator()
       ->free_phys(Config::PAGE_SHIFT, P_ptr <void> (iopde->ptabfn()));
      _mem_space->ram_quota()->free(Config::PAGE_SIZE);

      // free reference
      *iopde = 0;
    }
}

PUBLIC inline
Ram_quota *
Io_space::ram_quota() const
{ return _mem_space->ram_quota(); }

PRIVATE inline
bool
Io_space::is_superpage()
{ return _io_counter & 0x10000000; }

PUBLIC inline
void
Io_space::init (Mem_space* s)
{
  _mem_space = s;
}

// 
// Utilities for map<Io_space> and unmap<Io_space>
// 

PUBLIC inline
bool 
Io_space::v_fabricate (unsigned /*task_id*/, Address address, 
		       Address* phys, Address* size, unsigned* attribs = 0)
{
  if (_mem_space->is_sigma0())
    {
      // special-cased because we don't do lookup for sigma0
      *phys = address & ~(Map_superpage_size - 1);
      *size = Map_superpage_size;
      if (attribs) *attribs = Page_writable | Page_user_accessible;
      return true;
    }

  return false;
}

PUBLIC inline NEEDS[Io_space::is_superpage] 
bool 
Io_space::v_lookup (Address virt, Address *phys = 0, Address *size = 0,
		    unsigned *attribs = 0)
{
  if (is_superpage())
    {
      if (size) *size = Map_superpage_size;
      if (phys) *phys = 0;
      if (attribs) *attribs = Page_writable | Page_user_accessible;
      return true;
    }

  if (size) *size = 1;

  if (io_lookup (virt))
    {
      if (phys) *phys = virt;
      if (attribs) *attribs = Page_writable | Page_user_accessible;
      return true;
    }

  if (get_io_counter() == 0)
    {
      if (size) *size = Map_superpage_size;
      if (phys) *phys = 0;
    }

  return false;
}

PUBLIC inline NEEDS [Io_space::is_superpage]
unsigned long
Io_space::v_delete (Address virt, unsigned long size, 
		    unsigned long page_attribs = Page_all_attribs)
{
  (void)size;
  (void)page_attribs;
  assert (page_attribs == Page_all_attribs);

  if (is_superpage())
    {
      assert (size == Map_superpage_size);
      _io_counter = 0;
      return Page_writable | Page_user_accessible;
    }

  assert (size == 1);

  return io_delete (virt);
}

PUBLIC inline
Io_space::Status
Io_space::v_insert (Address phys, Address virt, size_t size, 
		    unsigned page_attribs)
{
  (void)phys;
  (void)size;
  (void)page_attribs;

  assert (phys == virt);
  if (get_io_counter() == 0 && size == Map_superpage_size)
    {
      _io_counter = 0x10000000 | Map_superpage_size;
      return Insert_ok;
    }
  
  assert (size == 1);

  return Io_space::Status (io_insert (virt));
}

PUBLIC inline
bool
Io_space::is_mappable (Address virt, size_t size)
{
  return virt < 0x10000U && (size == 1 || size == Map_superpage_size);
}

PUBLIC inline static
void
Io_space::tlb_flush ()
{}

PUBLIC inline static
bool 
Io_space::need_tlb_flush ()
{ return false; }

//
// IO lookup / insert / delete / counting
//

/** return the IO counter.
 *  @return number of IO ports mapped / 0 if not mapped
 */
PUBLIC inline NEEDS["paging.h"]
Mword
Io_space::get_io_counter() const
{
  return _io_counter & ~0x10000000;
}


/** Add something the the IO counter.
    @param incr number to add
    @pre 2nd level page table for IO bitmap is present
*/
inline NEEDS["paging.h"]
void
Io_space::addto_io_counter(int incr)
{
  atomic_add (&_io_counter, incr);
}


/** Lookup one IO port in the IO space.
    @param port_number port address to lookup; 
    @return true if mapped
     false if not 
 */
PROTECTED
bool
Io_space::io_lookup(Address port_number)
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
PROTECTED 
Io_space::Status
Io_space::io_insert(Address port_number)
{
  assert(port_number < L4_fpage::Io_port_max);

  // if SMAS is active, use the shadow IO bitmap since this space context is
  // not active and we switch the the IO bitmap at every context switch
  const Address ports_base = Config::Small_spaces
				   ? Mem_layout::Smas_io_bmap_bak
				   : Mem_layout::Io_bitmap;

  Address port_virt = ports_base + (port_number >> 3);
  Address port_phys = _mem_space->virt_to_phys (port_virt);

  if (port_phys == 0xffffffff)
    {
      // nothing mapped! Get a page and map it in the IO bitmap
      void *page;
      if (!(page=Mapped_allocator::allocator()->q_alloc(ram_quota(),
	      Config::PAGE_SHIFT)))
	return Insert_err_nomem;

      // clear all IO ports
      // bit == 1 disables the port
      // bit == 0 enables the port
      memset(page, 0xff, Config::PAGE_SIZE);

      Mem_space::Status status = 
	_mem_space->v_insert (Mem_layout::pmem_to_phys(page),
			      port_virt & Config::PAGE_MASK,
			      Config::PAGE_SIZE, Page_writable);

      if (status == Mem_space::Insert_err_nomem)
	{
	  Mapped_allocator::allocator()->free(Config::PAGE_SHIFT,page);
	  _mem_space->ram_quota()->free(Config::PAGE_SIZE);
	  return Insert_err_nomem;
	}

      // we've been careful, so insertion should have succeeded
      assert(status == Mem_space::Insert_ok); 

      port_phys = _mem_space->virt_to_phys (port_virt);
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
PROTECTED 
unsigned
Io_space::io_delete(Address port_number)
{
  assert(port_number < L4_fpage::Io_port_max);

  // be careful, do not cause page faults here
  // there might be nothing mapped in the IO bitmap

  Address port_addr = get_phys_port_addr(port_number);

  if (port_addr == 0xffffffff)
    // nothing mapped -> nothing to delete
    return 0;

  // so there is memory mapped in the IO bitmap -> disable the ports
  char * port = static_cast<char *> (Kmem::phys_to_virt(port_addr));

  // bit == 1 disables the port
  // bit == 0 enables the port
  if(!(*port & get_port_bit(port_number)))    // port enabled ??
    {
      *port |= get_port_bit(port_number);
      addto_io_counter(-1);

      return Page_writable | Page_user_accessible;
    }

  return 0;
}

INLINE NEEDS["config.h"]
Address
Io_space::get_phys_port_addr(Address const port_number) const
{
  const Address base = Config::Small_spaces ? Mem_layout::Smas_io_bmap_bak
					    : Mem_layout::Io_bitmap;
  return _mem_space->virt_to_phys(base + (port_number >> 3));
}

INLINE 
Unsigned8
Io_space::get_port_bit(Address const port_number) const
{
  return 1 << (port_number & 7);
}

