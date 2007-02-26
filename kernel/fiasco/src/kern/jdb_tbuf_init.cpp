INTERFACE:

#include "jdb_tbuf.h"
#include "observer.h"

class Jdb_tbuf_init : public Jdb_tbuf
{
public:
  static void init(Observer *);
};


IMPLEMENTATION:

#include <cassert>
#include <panic.h>

#include "vmem_alloc.h"
#include "config.h"
#include "boot_info.h"
#include "cpu.h"

// init trace buffer
IMPLEMENT FIASCO_INIT
void Jdb_tbuf_init::init(Observer *o)
{
  observer = o;

  if (!tbuf)
    {
      assert (sizeof(Tb_entry_fit64) == 64);

      unsigned n;
      
      // minimum: 8kB (  2 pages)
      // maximum: 2MB (512 pages)
      // must be a power of 2 (for performance reasons)
      for (n = 128;
	   n < Config::tbuf_entries && n*sizeof(Tb_entry_fit64)<0x200000;
	   n<<=1)
	;
      
      Config::tbuf_entries = n;

      unsigned size = n*sizeof(Tb_entry_fit64);
      
      if (! Vmem_alloc::page_alloc((void*)Kmem::tbuf_status_page, 0,
				   Vmem_alloc::NO_ZERO_FILL, Page::USER_RW))
	panic("jdb_tbuf: alloc status page at %08x failed", 
	      Kmem::tbuf_status_page);

      vm_offset_t va = Kmem::tbuf_buffer_area;
      for (unsigned i=0; i<size/Config::PAGE_SIZE; i++)
	{
	  if (! Vmem_alloc::page_alloc((void*)va, 0, 
				       Vmem_alloc::NO_ZERO_FILL, Page::USER_RW))
	    panic("jdb_tbuf: alloc buffer at %08x failed", va);
	  
	  va += Config::PAGE_SIZE;
	}

      status = (l4_tracebuffer_status_t *) Kmem::tbuf_status_page;
      status->tracebuffer0 = Kmem::tbuf_buffer_area;
      status->tracebuffer1 = Kmem::tbuf_buffer_area + size / 2;
      status->size0        =
      status->size1        = size / 2;
      status->version0     =
      status->version1     = 0;

      for (register int i = 0; i < LOG_EVENT_MAX_EVENTS; i++)
	{
#ifdef NO_LOG_EVENTS
	  status->logevents[i] = 0;
#else
	  // Note: constructors are called later, so don't try
	  // to validate here using log_events[i]->get_type() !
	  status->logevents[i] = (log_events[i]) ? 1 : 0;
#endif
	}

      status->scaler_tsc_to_ns = Cpu::get_scaler_tsc_to_ns();
      status->scaler_tsc_to_us = Cpu::get_scaler_tsc_to_us();
      status->scaler_ns_to_tsc = Cpu::get_scaler_ns_to_tsc();

      tbuf        = (Tb_entry_fit64*) Kmem::tbuf_buffer_area;
      tbuf_max    = tbuf + Config::tbuf_entries;
      count_mask1 =  Config::tbuf_entries    - 1;
      count_mask2 = (Config::tbuf_entries)/2 - 1;

      clear_tbuf();
    }
}
