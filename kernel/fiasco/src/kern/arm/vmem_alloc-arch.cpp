INTERFACE [arm]:

EXTENSION class Vmem_alloc
{
private:
  static P_ptr<void> zero_page;
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include "pagetable.h"
#include "kmem_space.h"
#include "mapped_alloc.h"
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
  void *zp = Mapped_allocator::allocator()->alloc(Config::PAGE_SHIFT);
  std::memset( zp, 0, Config::PAGE_SIZE );
  zero_page = P_ptr<void>(Mem_layout::pmem_to_phys((Address)zp));
  printf("  allocated zero page @%p[phys=%p]\n",
      zp,zero_page.get_raw());

  if(Config::VMEM_ALLOC_TEST) 
    {
      printf("Vmem_alloc::TEST\n");

      printf("  allocate zero-mapped page...");
      void *p = page_alloc((void*)(0xefc00000), ZERO_MAP );
      printf(" [%p] done\n",p);
      printf("  free zero-mapped page...");
      page_free(p);
      printf(" done\n");

      printf("  allocate zero-filled page...");
      p = page_alloc((void*)(0xefc01000), ZERO_FILL );
      printf(" [%p] done\n",p);
      printf("  free zero-filled page...");
      page_free(p);
      printf(" done\n");

      printf("  allocate no-zero-filled page...");
      p = page_alloc((void*)(0xefc02000), NO_ZERO_FILL );
      printf(" [%p] done\n",p);
      printf("  free no-zero-filled page...");
      page_free(p);
      printf(" done\n");
    }
}

IMPLEMENT
void *Vmem_alloc::page_alloc( void *address, Zero_fill zf, Page::Attribs pa )
{
  P_ptr<void> page;
  void *vpage;

  if (zf != ZERO_MAP)
    {
      vpage = Mapped_allocator::allocator()->alloc(Config::PAGE_SHIFT);
      if (!vpage) 
	{
	  kdb_ke("Vmem_alloc: can't alloc new page");
	  return 0;
	}
      page = P_ptr<void>(Mem_layout::pmem_to_phys((Address)vpage));
      Mem_unit::inv_dcache(vpage, ((char*)vpage) + Config::PAGE_SIZE);
    } 
  else 
    {
      page = zero_page;
      pa = Page::KERN_RO;
    }

  // insert page into master page table
  Page_table::Status status;
  status = Kmem_space::kdir()->replace(page, address, Config::PAGE_SIZE, pa, 
                                       true);
  //  Mem_unit::dtlb_flush(address);

  if(status==Page_table::E_EXISTS) 
    panic("Vmem_alloc: address already mapped");
  else if(status!=Page_table::E_OK) 
    panic("Vmem_alloc: some error mapping page");
  

  if (zf == ZERO_FILL)
    memset(address, 0, Config::PAGE_SIZE);

  return address;
}

IMPLEMENT 			   
void Vmem_alloc::page_free(void *page)
{
  P_ptr<void> phys = Kmem_space::kdir()->lookup(page,0,0);
  if (phys.is_null()) 
    return;

  Mem_unit::inv_dcache(page, ((char*)page) + Config::PAGE_SIZE);
  Kmem_space::kdir()->remove(page, true);
  
  if (phys != zero_page)
    Mapped_allocator::allocator()->free_phys(Config::PAGE_SHIFT, phys);

}

