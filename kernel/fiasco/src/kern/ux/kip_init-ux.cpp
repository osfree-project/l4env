IMPLEMENTATION [ux]:

#include "multiboot.h"

IMPLEMENT inline NEEDS ["kip.h", "kmem.h", "multiboot.h"] FIASCO_INIT
void
Kip_init::setup_arch()
{
  Multiboot_module *mbm = reinterpret_cast <Multiboot_module*>
    (Kmem::phys_to_virt (Boot_info::mbi_virt()->mods_addr));

  Kip::k()->sigma0_ip		= mbm->reserved;
  Kip::k()->sigma0_memory.low	= mbm->mod_start & Config::PAGE_MASK;
  Kip::k()->sigma0_memory.high	= ((mbm->mod_end + (Config::PAGE_SIZE-1))
				   & Config::PAGE_MASK);

  mbm++;
  Kip::k()->root_ip		= mbm->reserved;
  Kip::k()->root_memory.low	= mbm->mod_start & Config::PAGE_MASK;
  Kip::k()->root_memory.high	= ((mbm->mod_end + (Config::PAGE_SIZE-1))
				   & Config::PAGE_MASK);
}

IMPLEMENTATION [ux]:

IMPLEMENT inline NEEDS ["kip.h"] FIASCO_INIT
void
Kip_init::setup_arch_abi()
{
  Kip::k()->dedicated[0].low  = Kip::k()->root_memory.low;
  Kip::k()->dedicated[0].high = Kip::k()->root_memory.high;

  for (int i = 1; i < 4; i++)
    Kip::k()->dedicated[i].low = Kip::k()->dedicated[i].high = 0;         
}
