IMPLEMENTATION [ux]:

#include "boot_info.h"
#include "multiboot.h"
#include <cstring>

IMPLEMENT FIASCO_INIT
void
Kip_init::setup_arch()
{
  Multiboot_module *mbm = reinterpret_cast <Multiboot_module*>
    (Kmem::phys_to_virt (Boot_info::mbi_virt()->mods_addr));
  Kip::k()->user_ptr = (unsigned long)(Boot_info::mbi_phys());
  Mem_desc *m = Kip::k()->mem_descs();

  *(m++) = Mem_desc(0, Kmem::get_mem_max() - 1, Mem_desc::Conventional);
  *(m++) = Mem_desc(Kmem::kernel_image_start(), Kmem::kcode_end() - 1, 
      Mem_desc::Reserved);

  mbm++;
  Kip::k()->sigma0_ip		= mbm->reserved;
  *(m++) = Mem_desc(Boot_info::sigma0_start() & Config::PAGE_MASK,
                    ((Boot_info::sigma0_end() + (Config::PAGE_SIZE-1))
                     & Config::PAGE_MASK) - 1,
                    Mem_desc::Reserved);

  mbm++;
  Kip::k()->root_ip		= mbm->reserved;
  *(m++) = Mem_desc(Boot_info::root_start() & Config::PAGE_MASK,
                    ((Boot_info::root_end() + (Config::PAGE_SIZE-1))
                     & Config::PAGE_MASK) - 1,
                    Mem_desc::Bootloader);

  unsigned long version_size = 0;
  for (char const *v = Kip::k()->version_string(); *v; )
    {
      unsigned l = strlen(v) + 1;
      v += l;
      version_size += l;
    }

  version_size += 2;

  Kip::k()->vhw_offset = (Kip::k()->offset_version_strings << 4) + version_size;

  Kip::k()->vhw()->init();
}

IMPLEMENTATION [ux]:

IMPLEMENT inline NEEDS ["kip.h"] FIASCO_INIT
void
Kip_init::setup_arch_abi()
{
  unsigned long mod_start = ~0UL;
  unsigned long mod_end = 0;
  
  Mem_desc *m = Kip::k()->mem_descs();
  for (;m->type() != Mem_desc::Undefined; ++m)
    ;

  Multiboot_module *mbm = reinterpret_cast <Multiboot_module*>
    (Kmem::phys_to_virt (Boot_info::mbi_virt()->mods_addr));

  for (unsigned i = 3; i < Boot_info::mbi_virt()->mods_count; ++i)
    {
      if (mbm[i].mod_start < mod_start)
	mod_start = mbm[i].mod_start;

      if (mbm[i].mod_end > mod_end)
	mod_end = mbm[i].mod_end;
    }
  
  mod_start &= ~(Config::PAGE_SIZE - 1);
  mod_end = (mod_end + Config::PAGE_SIZE -1) & ~(Config::PAGE_SIZE - 1);

  if (mod_end > mod_start)
    *(m++) = Mem_desc(mod_start, mod_end - 1, Mem_desc::Bootloader);

  *(m++) = Mem_desc(Boot_info::mbi_phys(), 
      ((Boot_info::mbi_phys() + Boot_info::mbi_size() 
       + Config::PAGE_SIZE-1) & Config::PAGE_MASK) -1, 
      Mem_desc::Bootloader);
}
