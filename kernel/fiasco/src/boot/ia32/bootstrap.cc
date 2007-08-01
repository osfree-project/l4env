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
#include "config.h"
#include "globalconfig.h"
#include "kip.h"
#include "mem_layout.h"
#include "multiboot.h"
#include "panic.h"
#include "processor.h"
#include "reset.h"

struct check_sum
{
  char delimiter[16];
  Unsigned32 checksum_ro;
  Unsigned32 checksum_rw;
} check_sum = {"FIASCOCHECKSUM=", 0, 0};

extern "C" char _start[];
extern "C" char _end[];

extern "C" void exit(int rc) __attribute__((noreturn));

void
exit(int)
{
  for (;;)
    Proc::pause();
}

// test if [start1..end1-1] overlaps [start2..end2-1]
static
void
check_overlap (const char *str,
	       Address start1, Address end1, Address start2, Address end2)
{
  if ((start1 >= start2 && start1 < end2) || (end1 > start2 && end1 <= end2))
    panic("Kernel [0x%08lx,0x%08lx) overlaps %s [0x%08lx,0x%08lx).", 
	  start1, end1, str, start2, end2);
}

typedef void (*Start)(Multiboot_info *, unsigned, unsigned) FIASCO_FASTCALL;

extern "C" FIASCO_FASTCALL
void
bootstrap (Multiboot_info *mbi, unsigned int flag)
{
  extern Kip my_kernel_info_page;
  Address mem_max;
  Start start;

  assert(flag == Multiboot_header::Valid);

  // setup stuff for base_paging_init() 
  base_cpu_setup();
  // now do base_paging_init(): sets up paging with one-to-one mapping
  base_paging_init();

  asm volatile ("" ::: "memory");

  // this calculation must fit Kmem::init()!
  mem_max = (my_kernel_info_page.last_free().end + 1) & Config::PAGE_MASK;

  if (mem_max > (3048UL << 20))
    mem_max = 3048UL << 20;

  if (Config::old_sigma0_adapter_hack)
    {
      // limit memory to 1GB (Sigma0 protocol limitation)
      if (mem_max > 1<<30)
	mem_max = 1<<30;
    }

  // check if mem_max contains a bogus value (e.g. <4096KB)
  if (mem_max < 4<<20)
    panic("Need at least 4MB of RAM (mem_max=%u)", mem_max);

  // make sure that we did not forgot to discard an unused header section
  // (compare "objdump -p kernel.image")
  if ((Address)_start < Mem_layout::Kernel_image)
    panic("Fiasco kernel occupies memory below %08x",
	Mem_layout::Kernel_image);

  if ((Address)&_end - Mem_layout::Kernel_image > 4<<20)
    panic("Fiasco boot system occupies more than 4MB");

  base_map_physical_memory_for_kernel(round_superpage(mem_max));

  start = (Start)_start;

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
	      Address phys_start = (Address)_start 
		                   - Mem_layout::Kernel_image;
	      Address phys_end   = (Address)_end   
		                   - Mem_layout::Kernel_image;

	      check_overlap ("VGA/IO", phys_start, phys_end,
			     0xa0000, 0x100000);
#if 0 // I think our loader should already check overlaps with sigam0 or ...
	      check_overlap ("sigma0", phys_start, phys_end,
			     rki->sigma0_memory.start, rki->sigma0_memory.end);
	      check_overlap ("roottask", phys_start, phys_end,
			     rki->root_memory.start, rki->root_memory.end);
#endif
	    }
	}
    }

  if (Checksum::get_checksum_ro() != check_sum.checksum_ro)
    panic("Read-only (text) checksum does not match.");

  if (Checksum::get_checksum_rw() != check_sum.checksum_rw)
    panic("Read-write (data) checksum does not match.");

  start (mbi, flag, check_sum.checksum_ro);
}
