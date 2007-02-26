INTERFACE [ia32,ux,amd64]:

#include "initcalls.h"
#include "types.h"

class Kip_init
{
private:
  /// ABI specific part of KIP setup.
  static void setup_abi();

  /// Architecture specific part of KIP setup.
  static void setup_arch();

  /// Architecture and ABI specific KIP setup.
  static void setup_arch_abi();

public:
  /**
   * Insert memory descriptor for the Kmem region and finish the memory
   * info field.
   * @post no more memory descriptors may be added
   */
  static void setup_kmem_region (Address kmem_base, Address kmem_size);
};

IMPLEMENTATION [ia32,ux,amd64]:

#include <cstring>
#include "boot_info.h"
#include "config.h"
#include "cpu.h"
#include "div32.h"
#include "kip.h"
#include "kmem.h"
#include "panic.h"

static FIASCO_INIT
void
set_version_and_features()
{
#if 0
  extern char _initkip_start[], _initkip_end[];
  int list_size = _initkip_end - _initkip_start;

  if (list_size <= 512)
    memcpy(const_cast<char *>(Kip::k()->version_string()), _initkip_start,
           list_size);
  else
    panic("Kip: version string overflow");
#endif
}

/** KIP initialization. */
PUBLIC static FIASCO_INIT
void
Kip_init::init_x()
{
#if 0
  Kip::k()->magic		= L4_KERNEL_INFO_MAGIC;
  Kip::k()->version		= Config::kernel_version_id;
#endif
  Kip::k()->frequency_cpu	= div32(Cpu::frequency(), 1000);

  setup_arch();
  setup_abi();
  setup_arch_abi();

  set_version_and_features();
}

// =======================================================================
IMPLEMENTATION [ia32,ux,amd64]:

#include "config.h"
#include "panic.h"
#include "boot_info.h"
#include "kmem.h"


IMPLEMENT static inline NEEDS ["boot_info.h", "kmem.h"] FIASCO_INIT
void
Kip_init::setup_abi()
{
  Kip::k()->clock		= 0;
//  Kip::k()->main_memory.start	= 0;
//  Kip::k()->main_memory.end	= Kmem::get_mem_max();
//
//  Kip::k()->reserved0.start	= Kmem::kernel_image_start();
//  Kip::k()->reserved0.end	= Kmem::kcode_end();
//  Kip::k()->semi_reserved.start	= Boot_info::mbi_virt()->mem_lower << 10;
//  Kip::k()->semi_reserved.end	= 1 << 20;
  Kip::k()->sched_granularity	= Config::scheduler_granularity;
#if 0
  Kip::k()->offset_version_strings = ((Address)&Kip::k()->version_strings -
                                      (Address)&Kip::k()->magic         ) >> 4;
#endif
}

IMPLEMENT static inline FIASCO_INIT
void Kip_init::setup_kmem_region (Address, Address)
{
  Kip::k()->add_mem_region(Mem_desc(Kmem::kmem_base(),
	Kmem::get_mem_max() - 1, Mem_desc::Reserved));
}

namespace 
{
  enum 
  {
    Num_mem_descs = 20,
    Max_len_version = 512,

    Size_mem_descs = sizeof(Mword) * 2 * Num_mem_descs,
  };

  struct KIP
  {
    Kip kip;
    char mem_descs[Size_mem_descs];
  };
  
  KIP my_kernel_info_page __attribute__((section(".kernel_info_page"))) =
    { 
      { 
	L4_KERNEL_INFO_MAGIC, 
	Config::kernel_version_id, 
	(Size_mem_descs + sizeof(Kip)) >> 4, {0, 0, 0},
	0, {0,0,0}, // KIP SYSCALLS
	0, 0, 0, 0,
	0, 0, {},  0, 0, {},  0, 0, {},
	0, (sizeof(Kip) << (sizeof(Mword)*4)) | Num_mem_descs, 0, 0,
	0, {},
	0, 0,
	0, 0,
	0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0,
	Config::Vkey_irq,
	{}
      },
      {}
    };

};

PUBLIC static FIASCO_INIT
//IMPLEMENT
void Kip_init::init()
{
  Kip *kinfo = reinterpret_cast<Kip*>(&my_kernel_info_page);
  Kip::init_global_kip(kinfo);
  Kip_init::init_x();
  kinfo->add_mem_region(Mem_desc(0, Mem_layout::User_max - 1, 
	                  Mem_desc::Conventional, true));


  Mem_desc *md = kinfo->mem_descs();
  Mem_desc *end = md + kinfo->num_mem_descs();

  for (;md != end; ++md)
    {
      if (md->type() == Mem_desc::Reserved 
	  && !md->is_virtual()
	  && md->contains(Kmem::kernel_image_start())
	  && md->contains(Kmem::kcode_end()-1))
	{
	  *md = Mem_desc(Kmem::kernel_image_start(), Kmem::kcode_end() -1,
	      Mem_desc::Reserved);
	  break;
	}
    }
}
