/* this code is run directly from boot.S.  our task is to setup the
   paging just enough so that L4 can run in its native address space 
   at 0xf0001000, and then start up L4.  */

#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include "boot_cpu.h"
#include "boot_paging.h"
#include "checksum.h"
#include "globalconfig.h"
#include "kip.h"
#include "mem_layout.h"
#include "multiboot.h"
#include "reset.h"

struct check_sum
{
  char delimiter[16];
  unsigned long checksum_ro;
  unsigned long checksum_rw;
} check_sum = {"FIASCOCHECKSUM=", 0, 0};

extern "C" char _start[];
extern "C" char _end[];
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
      printf("\nPANIC: kernel [0x%08lx,0x%08lx) overlaps %s [0x%08lx,0x%08lx)", 
	    start1, end1, str, start2, end2);
      _exit(1);
    }
}

typedef void (*Start)(Multiboot_info *, unsigned, unsigned) FIASCO_FASTCALL;

extern "C" FIASCO_FASTCALL
void
bootstrap (Multiboot_info *mbi, unsigned int flag)
{
  Address mem_max;
  Start start;

  assert(flag == Multiboot_header::Valid);

  // setup stuff for base_paging_init() 
  base_cpu_setup();

  // this calculation must fit Kmem::init()!
  mem_max = trunc_page((mbi->mem_upper + 1024) << 10);
  if (mem_max > 1<<30)
    mem_max = 1<<30;

  // now do base_paging_init(): sets up paging with one-to-one mapping
  base_paging_init(round_superpage(mem_max));

  start = (Start)_start;
  
  // make sure that we did not forgot to discard an unused header section
  // (compare "objdump -p kernel.image")
  if ((unsigned long)_start < Mem_layout::Kernel_image)
    {
      printf("\nPANIC: kernel [%p,%p) occupies memory below %08x",
	   _start, _end, Mem_layout::Kernel_image);
      _exit(1);
    }

  // check if kernel overwrites something important
  if (mbi->flags & Multiboot_info::Cmdline)
    {
      char *sub = strstr(reinterpret_cast<const char*>
			 (mbi->cmdline), " proto=");
      if (sub)
	{
	  Kip *rki = (Kip*)strtoul(sub+7, 0, 0);
	  if (rki)
	    {
	      Address phys_start = (unsigned long)_start 
		                   - Mem_layout::Kernel_image;
	      Address phys_end   = (unsigned long)_end   
		                   - Mem_layout::Kernel_image;

	      check_overlap ("VGA/IO", phys_start, phys_end,
			     0xa0000, 0x100000);
	      check_overlap ("sigma0", phys_start, phys_end,
			     rki->sigma0_memory.low, rki->sigma0_memory.high);
	      check_overlap ("rmgr", phys_start, phys_end,
			     rki->root_memory.low, rki->root_memory.high);
	    }
	}
    }

  if (Checksum::get_checksum_ro() != check_sum.checksum_ro)
    {
      printf("\nPANIC: read-only (text) checksum does not match");
      _exit(1);
    }

  if (Checksum::get_checksum_rw() != check_sum.checksum_rw)
    {
      printf("\nPANIC: read-write (data) checksum does not match");
      _exit(1);
    }

  start (mbi, flag, check_sum.checksum_ro);
}
