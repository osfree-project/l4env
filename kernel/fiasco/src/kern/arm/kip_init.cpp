INTERFACE:

#include "kip.h"

class Kip
{
public:

  static Kernel_info *kip();
  static Kernel_memory_desc *mem_descs();
  static char const *const version_string();
  static void init();

};


IMPLEMENTATION:

#include <cstring>

#include "panic.h"
#include "boot_info.h"
#include "config.h"
#include "kmem.h"

IMPLEMENT
void Kip::init()
{
  Kernel_info *kinfo = Kmem::phys_to_virt(P_ptr<Kernel_info>(Boot_info::kip()));
  Kernel_info::init_kip( kinfo );

  if((Mword)kinfo < 0xf0000000) {
    panic("BAD: Kernel Info Page not in valid memory!!\n");
  }

  if(!kinfo->offset_memory_descs) {
    panic("BAD: No valid memory descriptors found in KIP\n");
  }
  kinfo->magic = L4_KERNEL_INFO_MAGIC;
  kinfo->version = Config::kernel_version_id;

  strcpy((char*)kinfo + (kinfo->offset_version_strings << 4),
	 Config::kernel_version_string);
  
  kinfo->clock = 0;

}

IMPLEMENT inline
Kernel_info *Kip::kip()
{
  return Kernel_info::kip();
}

IMPLEMENT inline
Kernel_memory_desc *Kip::mem_descs()
{
  return (Kernel_memory_desc*)((char*)Kernel_info::kip() + 
			       (Kernel_info::kip()->offset_memory_descs << 4));

}

IMPLEMENT inline
char const *const Kip::version_string()
{
  return (char const *)Kernel_info::kip() + 
    (Kernel_info::kip()->offset_version_strings << 4);

}

