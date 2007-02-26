/**
 * \file
 */

#include <stdio.h>
#include <l4/sys/kernel.h>
#include <l4/util/l4_macros.h>
#include <l4/env_support/panic.h>

#include "macros.h"
#include "init_kip.h"
#include "startup.h"
#include <l4/sys/kip.h>

using L4::Kip::Mem_desc;

extern unsigned char _stack;

#if defined(ARCH_x86) || defined(ARCH_amd64)
/**
 * setup Kernel Info Page
 */
void
init_kip_v2(void *_l4i, boot_info_t *bi, l4util_mb_info_t *mbi)
{
  l4_kernel_info_t *l4i = (l4_kernel_info_t *)_l4i;


  unsigned char l4_api = l4i->version >> 24;

  if (l4_api != 0x87 /*VERSION_FIASCO*/)
    panic("cannot load V2 kernels other than Fiasco");

  Mem_desc *md = Mem_desc::first(_l4i);
  (md++)->set(0, bi->mem_high - 1, Mem_desc::Conventional);
  (md++)->set(bi->kernel_low, bi->kernel_high - 1, Mem_desc::Reserved);
  (md++)->set(bi->sigma0_low, bi->sigma0_high - 1, Mem_desc::Reserved);
  (md++)->set(bi->roottask_low, bi->roottask_high - 1,Mem_desc::Bootloader);
  (md++)->set(l4_trunc_page(bi->mbi_low), l4_round_page(bi->mbi_high) - 1,
      Mem_desc::Bootloader);

  l4i->root_esp = (l4_umword_t) &_stack;
  /* don't add kernel, sigma0, and roottask to dedicated[1] because
   * these modules are already extracted once the kernel is running */
  int first_module = !!bi->kernel_high + !!bi->sigma0_high 
    + !!bi->roottask_high;

  (md++)->set(l4_trunc_page(0x9f000), l4_round_page(1<<20) - 1, 
      Mem_desc::Arch);

  (md++)->set(
      l4_trunc_page((L4_MB_MOD_PTR(mbi->mods_addr))[first_module].mod_start),
      l4_round_page((L4_MB_MOD_PTR(mbi->mods_addr))[mbi->mods_count-1].mod_end)-1,
      Mem_desc::Bootloader);


  /* set up sigma0 info */
  if (bi->sigma0_high)
    {
      l4i->sigma0_eip          = bi->sigma0_start;

      /* XXX UGLY HACK: Jochen's kernel can't pass args on a sigma0
         stack not within the L4 kernel itself -- therefore we use the
         kernel info page itself as the stack for sigma0.  the field
         we use is the task descriptor for the unused "ktest2" task */
      l4i->sigma0_esp = bi->sigma0_stack;
      l4i->user_ptr = (unsigned long)mbi;
      printf("  Sigma0 config    ip:"l4_addr_fmt" sp:"l4_addr_fmt"\n",
	     l4i->sigma0_eip, l4i->sigma0_esp);
    }

  /* set up roottask info */
  if (bi->roottask_high)
    {
      l4i->root_eip          = bi->roottask_start;
      printf("  Roottask config  ip:"l4_addr_fmt" sp:"l4_addr_fmt"\n",
	     l4i->root_eip, l4i->root_esp);
    }
}
#endif
