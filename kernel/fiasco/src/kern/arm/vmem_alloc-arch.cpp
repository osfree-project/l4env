INTERFACE:

EXTENSION class Vmem_alloc
{
private:
  static P_ptr<void> zero_page;
};


IMPLEMENTATION[arch]:

#include "pagetable.h"
#include "kmem_space.h"
#include "kmem_alloc.h"
#include "config.h"
#include "panic.h"
#include "kdb_ke.h"
#include "mem_unit.h"
#include "static_init.h"

#include <cstdio>
#include <cassert>
#include <cstring>


// this static initializer must have a higer priotity than Vmem_alloc::init()
P_ptr<void> Vmem_alloc::zero_page INIT_PRIORITY(MAX_INIT_PRIO);


IMPLEMENT
void Vmem_alloc::init()
{
  // Allocate a generic zero page
  printf("Vmem_alloc::init()\n");
  void *zp = Kmem_alloc::allocator()->alloc(0);
  std::memset( zp, 0, Config::PAGE_SIZE );
  zero_page = Kmem_alloc::allocator()->virt_to_phys(zp);
  printf("  allocated zero page @%p[phys=%p]\n",
	 zp,zero_page.get_raw());

  if(Config::VMEM_ALLOC_TEST) {
    printf("Vmem_alloc::TEST\n");

    printf("  allocate zero-mapped page...");
    void *p = page_alloc((void*)(0xefc00000), 0/*ignore*/, ZERO_MAP );
    printf(" [%p] done\n",p);
    printf("  free zero-mapped page...");
    page_free(p, 0/*ignore*/);
    printf(" done\n");

    printf("  allocate zero-filled page...");
    p = page_alloc((void*)(0xefc01000), 0/*ignore*/, ZERO_FILL );
    printf(" [%p] done\n",p);
    printf("  free zero-filled page...");
    page_free(p, 0/*ignore*/);
    printf(" done\n");

    printf("  allocate no-zero-filled page...");
    p = page_alloc((void*)(0xefc02000), 0/*ignore*/, NO_ZERO_FILL );
    printf(" [%p] done\n",p);
    printf("  free no-zero-filled page...");
    page_free(p, 0/*ignore*/ );
    printf(" done\n");
  }
}



IMPLEMENT
void *Vmem_alloc::page_alloc( void *address, int order, Zero_fill zf,
			      Page::Attribs pa )

{
  //  assert((vm_offset_t)address >= kmem::mem_user_max);

  P_ptr<void> page;
  void *vpage;
  
  if (zf != ZERO_MAP) {
    vpage = Kmem_alloc::allocator()->alloc(0);
    if (!vpage) {
      kdb_ke("Vmem_alloc: can't alloc new page");
      return 0;
    }
    page = Kmem_alloc::allocator()->virt_to_phys(vpage);
  } else {
    page = zero_page;
  }
  
  // insert page into master page table
  Page_table::Status status;
  status = Kmem_space::kdir()->insert( page, address, Config::PAGE_SIZE, pa );
  
  if(status==Page_table::E_EXISTS) {
    panic("Vmem_alloc: address already mapped");
  } else if(status!=Page_table::E_OK) {
    panic("Vmem_alloc: some error mapping page");
  }
     
  if (zf == ZERO_FILL)
    memset(address, 0, Config::PAGE_SIZE);

  return address;
}



IMPLEMENT 			   
void Vmem_alloc::page_free( void *page, int order )
{
  P_ptr<void> phys = Kmem_space::kdir()->lookup(page,0,0);
  if (phys.is_null()) 
    return;

  if (phys != zero_page)
    Kmem_alloc::allocator()->free( 0, phys );

  Kmem_space::kdir()->remove( page );

}

