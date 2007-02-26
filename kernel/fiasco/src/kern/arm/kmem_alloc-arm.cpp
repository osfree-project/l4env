IMPLEMENTATION[arm]:

#include <cstdio>

#include "config.h"
#include "helping_lock.h"
#include "kip_init.h"
#include "kmem.h"
#include "lmm.h"
#include "panic.h"

static lmm_region_t lmm_region_all;

IMPLEMENT
Kmem_alloc::Kmem_alloc()
{
  size_t mem_size = Kip::kip()->total_ram * Config::kernel_mem_per_cent / 100;
  mem_size &= ~(Config::PAGE_SIZE -1);
  
  printf("Kmem_alloc::Kmem_alloc() [lmm=%p]\n", lmm);

  lmm_init((lmm_t*)lmm);
  lmm_add_region((lmm_t*)lmm, &lmm_region_all, (void*)0, (vm_size_t)-1, 0, 0);

  Kernel_memory_desc *md = Kip::mem_descs();
  Kernel_memory_desc *amd = md;
  while(md->size_type) md++;
  assert(md != amd);
  do {
    md--;
    if((md->size_type & L4_MEMORY_DESC_TYPE_MASK) == L4_MEMORY_DESC_AVAIL) {
      size_t s = md->size_type & L4_MEMORY_DESC_SIZE_MASK;
      char *virt = Kmem::phys_to_virt(P_ptr<char>((char*)md->base));
      if(s >= mem_size) {
	lmm_add_free((lmm_t*)lmm, virt + ( s - mem_size ), mem_size );
	mem_size = 0;
	break;
      } else {
	lmm_add_free((lmm_t*)lmm, virt , s );
	mem_size -= s;
      }
    }
  } while(md!=amd && mem_size > 0);
  if(mem_size > 0)
    panic("Could not get the memory necessary for the kernel\n");

  printf("Kmem_alloc() finished!\n"
	 "  avail mem = %d Kbytes\n", lmm_avail((lmm_t*)lmm,0)>>10);
 
}


IMPLEMENT
void *Kmem_alloc::_phys_to_virt( void *phys ) const
{
  return (char*)phys + (Kmem::PHYS_MAP_BASE - Kmem::PHYS_SDRAM_BASE);
}

IMPLEMENT
void *Kmem_alloc::_virt_to_phys( void *virt ) const
{
  return (char*)virt - (Kmem::PHYS_MAP_BASE - Kmem::PHYS_SDRAM_BASE);

}

