/* this code is run directly from boot.S.  our task is to setup the
   paging just enough so that L4 can run in its native address space 
   at 0xf0001000, and then start up L4.  */

#include <cassert>
#include <cstring>
#include <cstdio>

#include <flux/x86/multiboot.h>
#include <flux/x86/base_paging.h>
#include <flux/x86/base_cpu.h>
#include <flux/x86/pc/phys_lmm.h>

#include "load_elf.h"

extern "C" unsigned checksum_ro;
extern "C" unsigned checksum_rw;
extern "C" char _binary_kernel_start;

extern "C"
void
bootstrap (struct multiboot_info *mbi, unsigned int flag)
{
  void (*start)(struct multiboot_info *, unsigned, unsigned, unsigned);

  assert(flag == MULTIBOOT_VALID);

  // setup stuff for base_paging_init() 
  base_cpu_setup();
  phys_mem_max = 1024 * (1024 + mbi->mem_upper);

  if(phys_mem_max > (256<<20)) 
    {
      puts("WARNING: More than 256MB RAM found, limiting hard to 256MB");
      phys_mem_max = 256 << 20;
    }

  // now do base_paging_init(): sets up paging with one-to-one mapping
  base_paging_init();

  // map in physical memory at 0xf0000000
  pdir_map_range(base_pdir_pa, 0xf0000000, 0, phys_mem_max,
		 INTEL_PTE_VALID | INTEL_PDE_WRITE | INTEL_PDE_USER);

  start = (void (*)(multiboot_info *, unsigned, unsigned, unsigned))
    load_elf("kernel", &_binary_kernel_start, 0, 0);

  start (mbi, flag, (unsigned) &checksum_ro, (unsigned) &checksum_rw);
}

/* the following simple-minded function definition overrides the (more
   complicated) default one the OSKIT*/
extern "C"
int
ptab_alloc(vm_offset_t *out_ptab_pa)
{
  static char pool[0x100000];
  static vm_offset_t pdirs;
  static int initialized;

  if (! initialized)
    {
      initialized = 1;
      memset( pool, 0, sizeof(pool) );
      // printf("pool from %p to %p\n",pool, ((char*)pool) + sizeof(pool));

      pdirs = (reinterpret_cast<vm_offset_t>(pool) + PAGE_SIZE - 1) 
	& ~PAGE_MASK;
    }

  assert(pdirs < reinterpret_cast<vm_offset_t>(pool) + sizeof(pool));

  *out_ptab_pa = pdirs;
  pdirs += PAGE_SIZE;

  return 0;
}
