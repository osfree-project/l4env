/* this code is run directly from boot.S.  our task is to setup the
   paging just enough so that L4 can run in its native address space 
   at 0xf0001000, and then start up L4.  */

#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <flux/x86/multiboot.h>
#include <flux/x86/base_paging.h>
#include <flux/x86/base_cpu.h>
#include <flux/x86/pc/phys_lmm.h>

#include "globalconfig.h"
#include "load_elf.h"
#include "kip.h"
#include "reset.h"


#define PAGE_SIZE	4096
#define PAGE_MASK	(PAGE_SIZE-1)
#define SUPERPAGE_SIZE  (4096*1024)
#define SUPERPAGE_MASK  (SUPERPAGE_SIZE-1)


extern "C" unsigned checksum_ro;
extern "C" unsigned checksum_rw;
extern "C" char _binary_kernel_start;
extern "C" void _exit(int code) __attribute__((noreturn));

// test if [start1..end1-1] overlaps [start2..end2-1]
static
void
check_overlap (const char *str,
	       Address start1, Address end1, Address start2, Address end2)
{
  if (   (start1 >= start2 && start1 <  end2)
      || (end1   >  start2 && end1   <= end2))
    {
      printf("\nPANIC: kernel (%08x-%08x) overlaps %s (%08x-%08x)", 
	    start1, end1, str, start2, end2);
      _exit(1);
    }
}

extern "C"
void
bootstrap (struct multiboot_info *mbi, unsigned int flag)
{
  void (*start)(struct multiboot_info *, unsigned, unsigned, unsigned);
  Address vma_start, vma_end;

  assert(flag == MULTIBOOT_VALID);

  // setup stuff for base_paging_init() 
  base_cpu_setup();

  phys_mem_max = 1024 * (1024 + mbi->mem_upper);
  phys_mem_max = (phys_mem_max + SUPERPAGE_MASK) & ~SUPERPAGE_MASK;

  if (phys_mem_max > (256<<20)) 
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
    load_elf("kernel", &_binary_kernel_start, &vma_start, &vma_end);

  // make sure that we did not forgot to discard an unused header section
  // (compare "objdump -p kernel.image")
  if (vma_start < 0xf0000000)
    {
      printf("\nPANIC: kernel (%08x-%08x) occupies memory below 0xf0000000",
	    vma_start, vma_end);
      _exit(1);
    }

  // check if kernel overwrites something important
  if (mbi->flags & MULTIBOOT_CMDLINE)
    {
      char *sub = strstr((const char*)(mbi->cmdline), " proto=");
      if (sub)
	{
	  Kernel_info *rki = (Kernel_info*)strtoul(sub+7, 0, 0);
	  if (rki)
	    {
	      Address phys_start = vma_start - 0xf0000000;
	      Address phys_end   = vma_end   - 0xf0000000;

	      check_overlap("sigma0", phys_start, phys_end,
			    rki->sigma0_memory.low, rki->sigma0_memory.high);
	      check_overlap("rmgr", phys_start, phys_end,
			    rki->sigma0_memory.low, rki->sigma0_memory.high);
	    }
	}
    }

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
