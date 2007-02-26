/* startup stuff */

/* it should be possible to throw away the text/data/bss of the object
   file resulting from this source -- so, we don't define here
   anything we could still use at a later time.  instead, globals are
   defined in globals.c */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <flux/machine/multiboot.h>
#include <flux/page.h>

#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>

#include "config.h"
#include "memmap.h"
#include "globals.h"

#include "init.h"

static void init_globals(void);
static void init_memmap(void);

/* started as the L4 sigma0 task from crt0.S */
void 
init(struct multiboot_info *mbi, unsigned int flag,
     l4_kernel_info_t *info)
{
  vm_offset_t address;

  printf("SIGMA0: Hello!\n");

  l4_info = info;

  if(l4_info->version == 0x01004444)
    {
      char kip_syscalls = l4_info->reserved[3];
      printf("SIGMA0: Found Fiasco, test for KIP syscalls: %s\n",
	     kip_syscalls ? "yes" : "no");

      if(kip_syscalls == 0)
	{
#       ifdef KIP_SYSCALLS
	  puts("SIGMA0:   KIP syscalls are not supported by the kernel");
	  panic("SIGMA0:   System stopped");
#       endif
	}
      else
	{

	  printf("SIGMA0: kip                    @%p\n"
		 "SIGMA0: sys_ipc                @%p\n"
		 "SIGMA0: sys_id_nearest         @%p\n"
		 "SIGMA0: sys_fpage_unmap        @%p\n"
		 "SIGMA0: sys_thread_switch      @%p\n"
		 "SIGMA0: sys_thread_schedule    @%p\n"
		 "SIGMA0: sys_lthread_ex_regs    @%p\n"
		 "SIGMA0: sys_task_new           @%p\n",
		 l4_info, 
		 (char *)l4_info + l4_info->sys_ipc,
		 (char *)l4_info + l4_info->sys_id_nearest,
		 (char *)l4_info + l4_info->sys_fpage_unmap,
		 (char *)l4_info + l4_info->sys_thread_switch,
		 (char *)l4_info + l4_info->sys_thread_schedule,
		 (char *)l4_info + l4_info->sys_lthread_ex_regs,
		 (char *)l4_info + l4_info->sys_task_new);
	}
    }

  init_globals();
  init_memmap();

  /* add the memory used by this module and not any longer required
     (the ".init" section) to the free memory pool */
  for (address = (vm_offset_t) &__crt_dummy__ & L4_PAGEMASK;
       address < ((vm_offset_t) &_stext & L4_PAGEMASK);
       address += L4_PAGESIZE)
    {
      check(memmap_free_page(address, O_RESERVED));
    }

  /* now start the memory manager */
  pager();
}

/* support functions for init() */


static void init_globals(void)
{
  l4_umword_t dummy;

  /* set some globals */

  /* set myself (my thread id) */
  myself = l4_myself();

  /* set my_pager */
  my_preempter = my_pager = L4_INVALID_ID;
  l4_thread_ex_regs(myself, (l4_umword_t) -1, (l4_umword_t) -1, 
		    &my_preempter, &my_pager, &dummy, &dummy, &dummy);

  /* bug compatibility with Jochen's L4 */
  jochen = ! (l4_info->version & 0xffff0000); /* jochen's versions have
						 high-word unset */
}

static void 
init_memmap(void)
{
  vm_offset_t address;
  int i;
  
  /* initialize memory pages to "reserved" and io ports to free */
  map_init();

  /* find the kernel info page */
  assert(l4_info->magic == L4_KERNEL_INFO_MAGIC);

  mem_high = l4_info->main_memory.high & ~PAGE_MASK;

  if (mem_high > MEM_MAX)
    {
      printf("SIGMA0: WARNING: all memory above %lu MB is wasted!\n"
	     "        Press any key to continue...\r", MEM_MAX >> 20);
#ifndef FIASCO_UX	    
      getchar();
#endif      
      printf("\r                                       \r");
      mem_high = MEM_MAX;
    }

  /* free all non-reserved memory: first, free all, then reserve stuff */
  for (address = trunc_page(l4_info->main_memory.low); 
       address < round_page(l4_info->main_memory.high); 
       address += L4_PAGESIZE)
    {
      memmap_free_page(address, O_RESERVED);
    }

  for (address = trunc_page(l4_info->reserved0.low);
       address < round_page(l4_info->reserved0.high);
       address += L4_PAGESIZE)
    {
      memmap_alloc_page(address, O_RESERVED);
    }

  for (address = trunc_page(l4_info->reserved1.low);
       address < round_page(l4_info->reserved1.high);
       address += L4_PAGESIZE)
    {
      memmap_alloc_page(address, O_RESERVED);
    }

  for (address = trunc_page(l4_info->semi_reserved.low);
       address < round_page(l4_info->semi_reserved.high);
       address += L4_PAGESIZE)
    {
      memmap_alloc_page(address, O_RESERVED);
    }

  assert(l4_info->sigma0_memory.low <= (vm_offset_t)&_stext);
  assert(l4_info->sigma0_memory.high >= (vm_offset_t)&_end);

  for (address = trunc_page(l4_info->sigma0_memory.low);
       address < round_page(l4_info->sigma0_memory.high);
       address += L4_PAGESIZE)
    {
      memmap_alloc_page(address, O_RESERVED);
    }

  for (address = trunc_page(l4_info->sigma1_memory.low);
       address < round_page(l4_info->sigma1_memory.high);
       address += L4_PAGESIZE)
    {
      memmap_alloc_page(address, O_RESERVED);
    }

  for (i = 0; i < 4; i++)
    for (address = trunc_page(l4_info->dedicated[i].low);
	 address < round_page(l4_info->dedicated[i].high);
	 address += L4_PAGESIZE)
      {
	memmap_alloc_page(address, ROOT_TASKNO);
      }

  for (address = trunc_page(l4_info->root_memory.low);
       address < round_page(l4_info->root_memory.high);
       address += L4_PAGESIZE)
    {
      memmap_alloc_page(address, ROOT_TASKNO);
    }

  /* the non-RAM high-memory superpages are all free for allocation */
  for (address = 0x8000; address < 0x10000;
       address += L4_SUPERPAGESIZE/0x10000) /* scaled by 0x10000**-1 to
					       prevent overflow */
    {
      memmap_free_superpage(address * 0x10000, O_RESERVED);
    }

  return;
}

