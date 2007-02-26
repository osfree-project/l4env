IMPLEMENTATION[ia32-v4]:

#include "cmdline.h"

/// v4 ABI specific KIP initialization
IMPLEMENT FIASCO_INIT
void Kmem::setup_kip_abi (Kernel_info *kinfo)
{
  extern char __crt_dummy__, _end; // defined by linker and in crt0.S

  // mark the kernel code area as reserved
  kinfo->mem_desc_add (virt_to_phys(&__crt_dummy__) & Config::PAGE_MASK,
		       (virt_to_phys(&_end) + Config::PAGE_SIZE -1)
		       & Config::PAGE_MASK,
		       2, 0, 0);

  // mark the area 640K...1M as shared
  kinfo->mem_desc_add (1024 * Boot_info::mbi_virt()->mem_lower,
		       1024 * 1024,
		       4, 0, 0);

  // the KIP size is one page
  kinfo->kip_area_info = Config::PAGE_SHIFT;
  
  // report that our hardware supports 4K and 4M pages with
  // read and write rights
  kinfo->page_info = Config::PAGE_SIZE | Config::SUPERPAGE_SIZE | 6;

  // set size of processor descriptions to 2^2 bytes and initialize pointer
  kinfo->processor_info = 2 << 28;
  kinfo->processor_desc_ptr = (char*) & (kinfo->frequency_bus) - (char*) kinfo;

  // user thread IDs start at 0x200,
  // system thread IDs start at the kernel thread ID,
  // we support max. 2^14 threads
  kinfo->thread_info (0x200, Config::KERNEL_ID, 14);

  // fill out the utcb_info field with values from Config
  kinfo->utcb_info (Config::MIN_UTCB_AREA_SIZE, 
		    Config::UTCB_ALIGNMENT,
		    Config::UTCB_SIZE_MULTIPLIER);
}

