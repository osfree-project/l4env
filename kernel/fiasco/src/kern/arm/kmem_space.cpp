INTERFACE:

#include "kmem.h"

class Page_table;

class Kmem_space : public Kmem
{

public:
  

  static void init();

  static Page_table *kdir();

private:

  static Page_table *_kdir;


};

IMPLEMENTATION:

#include <cassert>
#include <panic.h>

#include "pagetable.h"
#include "kmem_alloc.h"
#include "kmem.h"
#include "kip_init.h"

Page_table *Kmem_space::_kdir;

IMPLEMENT
Page_table *Kmem_space::kdir()
{
  return _kdir;
}

static
Page_table::Status insert_mapping( Page_table *pt, P_ptr<void> phys, 
				   void* virt, size_t size,
				   bool no_cache = false)
{
  size_t const num_ps = Page_table::num_page_sizes();
  size_t const * const sizes = Page_table::page_sizes();

  P_ptr<char> aphys = P_ptr<char>::cast(phys);
  char *avirt = (char*)virt;
  assert(num_ps>0);
  // alignment test
  assert(((aphys.get_unsigned() | (Unsigned32)avirt | size) & (sizes[0]-1)) == 0);
  size_t ps = num_ps -1;
  while(size > 0) {
    if((((aphys.get_unsigned() | (Unsigned32)avirt) & (sizes[ps]-1)) == 0 )
       && (size >= sizes[ps])) {

      //      printf("insert %08x mapping @ %p\n",sizes[ps],avirt);
      if(no_cache) {
	Page_table::Status s =
	  pt->insert( P_ptr<void>::cast(aphys), (void*)avirt, sizes[ps],
		      Page::USER_RWX | Page::NONCACHEABLE );
	if(s!=Page_table::E_OK) return s;
      } else {
	Page_table::Status s =
	  pt->insert( P_ptr<void>::cast(aphys), (void*)avirt, sizes[ps],
		      Page::USER_RWX | Page::CACHEABLE);
	if(s!=Page_table::E_OK) return s;
      }
     
      size  -= sizes[ps];
      aphys += sizes[ps];
      avirt += sizes[ps];
      ps = num_ps -1;
    } else {
      --ps;
    }
  }
  return Page_table::E_OK;
}



#include "console.h"


// initialze the kernel space (page table)
IMPLEMENT
void Kmem_space::init()
{

  Page_table::set_allocator(Kmem_alloc::allocator());

  Page_table *pt = new Page_table();
  _kdir = pt;

  Page_table::Status s;

  s = insert_mapping( pt, P_ptr<void>(PHYS_SDRAM_BASE), 
		      (void*)Kmem::PHYS_MAP_BASE, Kip::kip()->total_ram );
  if(s!=Page_table::E_OK) {
    panic("FATAL: error mapping linear phys area\n");
  }


  // use 4KB page for uart adapter mem
  s = insert_mapping( pt, P_ptr<void>(PHYS_UART3_BASE),
		      (void*)Kmem::UART3_MAP_BASE, 4096, true );

  if(s!=Page_table::E_OK) {
    panic("FATAL: error mapping uart registers\n");
  }


  s = insert_mapping( pt, P_ptr<void>(PHYS_FLUSH_BASE),
		      (void*)Kmem::CACHE_FLUSH_AREA, 64*1024 );

  if(s!=Page_table::E_OK) {
    panic("FATAL: error mapping cache flush area: %d\n",(unsigned)s);
  }

  // use 1MB page for hardware register mem
  s = insert_mapping( pt, P_ptr<void>(PHYS_HW_REGS_BASE),
		      (void*)Kmem::HW_REGS_MAP_BASE, 1024*1024, true );

  if(s!=Page_table::E_OK) {
    panic("FATAL: error mapping hardware registers registers\n");
  }


  Page_table::init();
  
  // disable console because with a new page table
  // the console driver needs to be reinitialized.
  // The boot console driver is no logner usable.
  Console::disable_all();

  pt->activate();


}
