INTERFACE [arm]:

EXTENSION class Vmem_alloc
{
private:
  static Address zero_page;
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
Address Vmem_alloc::zero_page;

IMPLEMENT
void Vmem_alloc::init()
{
  // Allocate a generic zero page
  printf("Vmem_alloc::init()\n");
  void *zp = Mapped_allocator::allocator()->alloc(Config::PAGE_SHIFT);
  std::memset( zp, 0, Config::PAGE_SIZE );
  zero_page = Kmem_space::kdir()->walk(zp,0,false).phys(zp);
  printf("  allocated zero page @%p[phys=%p]\n",
      zp, (void*)zero_page);

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
  Address page;
  void *vpage;

  if (zf != ZERO_MAP)
    {
      vpage = Mapped_allocator::allocator()->alloc(Config::PAGE_SHIFT);
      if (!vpage) 
	{
	  kdb_ke("Vmem_alloc: can't alloc new page");
	  return 0;
	}
      page = Kmem_space::kdir()->walk(vpage, 0, false).phys(vpage);
      //printf("  allocated page (virt=%p, phys=%08lx\n", vpage, page);
      Mem_unit::inv_dcache(vpage, ((char*)vpage) + Config::PAGE_SIZE);
    } 
  else 
    {
      page = zero_page;
      pa = Page::KERN_RO;
    }

#if 0
  printf("  phys=%08lx\n", page);
  printf("  address=%p\n", address);
#endif
  // insert page into master page table
  Pte pte = Kmem_space::kdir()->walk(address, Config::PAGE_SIZE, true);
  pte.set(page, Config::PAGE_SIZE, pa | Page::CACHEABLE, true);

  Mem_unit::dtlb_flush(address);

  if (zf == ZERO_FILL)
    memset(address, 0, Config::PAGE_SIZE);

  return address;
}

IMPLEMENT 			   
void Vmem_alloc::page_free(void *page)
{
  Pte pte = Kmem_space::kdir()->walk(page, 0, false);
  if (!pte.valid())
    return;

  // Invalidate the page because we remove this alias and use the
  // mapped_allocator mapping from now on.
  // Write back is not needed here, because we free the page.
  Mem_unit::inv_vdcache(page, ((char*)page) + Config::PAGE_SIZE);

  Address phys = pte.phys(page);
  pte.set_invalid(0, true);
  Mem_unit::dtlb_flush(page);
  
  if (phys != zero_page)
    Mapped_allocator::allocator()->free_phys(Config::PAGE_SHIFT, (void*)phys);

}

