#include <l4/sys/kernel.h>
#include <l4/sys/kip.h>
#include "init_kip.h"
#include <stdio.h>
#include <l4/env_support/panic.h>
#include "startup.h"

using L4::Kip::Mem_desc;

void init_kip_arm(void *k, boot_info_t *bi, l4util_mb_info_t *mbi)
{
  typedef void (*startup_func)(void);
  startup_func f = (startup_func)bi->kernel_start;
  l4_kernel_info_t *kip = (l4_kernel_info_t *)k;
  kip->user_ptr = (unsigned long)mbi;

  kip->sigma0_eip = bi->sigma0_start;
  kip->root_eip   = bi->roottask_start;

  Mem_desc *md = Mem_desc::first(kip);
  (md++)->set(RAM_BASE, RAM_BASE + ((MEMORY << 20) - 1),
      Mem_desc::Conventional);
  (md++)->set(bi->kernel_low, bi->kernel_high - 1, Mem_desc::Reserved);
  (md++)->set(bi->sigma0_low, bi->sigma0_high - 1, Mem_desc::Reserved);
  (md++)->set(bi->roottask_low, bi->roottask_high - 1,Mem_desc::Bootloader);
  (md++)->set(l4_trunc_page(bi->mbi_low), l4_round_page(bi->mbi_high) - 1,
      Mem_desc::Bootloader);

  unsigned long module_data_start = ~0UL;
  unsigned long module_data_end   = 0UL;

  l4util_mb_mod_t *modules = (l4util_mb_mod_t*)(mbi->mods_addr);

  for (unsigned long i = 0; i < mbi->mods_count; ++i)
    {
      if (modules[i].mod_start < module_data_start)
	module_data_start = modules[i].mod_start;
      if (modules[i].mod_end > module_data_end)
	module_data_end = modules[i].mod_end;
    }

  if (module_data_start < module_data_end)
    {
      (md++)->set((l4_umword_t)module_data_start,
                  (l4_umword_t)module_data_end - 1, Mem_desc::Bootloader);
    }

#if 0 // print memory regions from KIP
  for (md = Mem_desc::first(kip); 
       md != Mem_desc::first(kip) + Mem_desc::count(kip); ++md)
    printf("%p: %lx-%lx t=%x\n", md, md->start(), md->end(), md->type());
#endif


  printf("Starting kernel... (%x)\n", 0);
  f();
  panic("Returned from kernel?!");
}
