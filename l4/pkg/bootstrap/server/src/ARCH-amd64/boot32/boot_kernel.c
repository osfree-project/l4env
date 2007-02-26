#include <stdio.h>
#include <l4/util/mb_info.h>
#include "boot_cpu.h"
#include "boot_paging.h"
#include "load_elf.h"

extern unsigned KERNEL_CS_64;
extern char _binary_bootstrap64_bin_start;

void bootstrap (l4util_mb_info_t *mbi, unsigned int flag, char *rm_pointer);
void
bootstrap (l4util_mb_info_t *mbi, unsigned int flag, char *rm_pointer)
{
  l4_uint32_t vma_start, vma_end;
  struct
  {
    l4_uint32_t start;
    l4_uint16_t cs __attribute__((packed));
  } far_ptr;
  l4_mword_t mem_upper;

  // setup stuff for base_paging_init() 
  base_cpu_setup();

#ifdef REALMODE_LOADING
  mem_upper = *(unsigned long*)(rm_pointer + 0x1e0);
#else
  mem_upper = mbi->mem_upper = mbi->mem_upper;
#endif

  // now do base_paging_init(): sets up paging with one-to-one mapping
  base_paging_init(round_superpage(1024 * (1024 + mem_upper)));

  // switch from 32 Bit compatibility mode to 64 Bit mode
  far_ptr.cs    = KERNEL_CS_64;
  far_ptr.start = load_elf(&_binary_bootstrap64_bin_start, 
  			   &vma_start, &vma_end);

  asm volatile("ljmp *(%3)"
                :: "D"(mbi), "S"(flag), "d"(rm_pointer), 
		   "rm"(&far_ptr), "m"(far_ptr));
}
