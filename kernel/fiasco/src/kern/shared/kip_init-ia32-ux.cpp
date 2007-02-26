INTERFACE [ia32,ux]:

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

IMPLEMENTATION [ia32,ux]:

#include <string.h>

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
  extern char _initkip_start[], _initkip_end[];
  int list_size = _initkip_end - _initkip_start;

  if (list_size <= 512)
    memcpy(const_cast<char *>(Kip::k()->version_string()), _initkip_start,
           list_size);
  else
    panic("Kip: version string overflow");
}

/** KIP initialization. */
PUBLIC static FIASCO_INIT
void
Kip_init::init()
{
  Kip::k()->magic		= L4_KERNEL_INFO_MAGIC;
  Kip::k()->version		= Config::kernel_version_id;
  Kip::k()->frequency_cpu	= div32(Cpu::frequency(), 1000);

  setup_arch();
  setup_abi();
  setup_arch_abi();

  set_version_and_features();
}

// =======================================================================
IMPLEMENTATION [ia32,ux]:

IMPLEMENT static inline NEEDS ["boot_info.h", "kmem.h"] FIASCO_INIT
void
Kip_init::setup_abi()
{
  Kip::k()->clock		= 0;
  Kip::k()->main_memory.low	= 0;
  Kip::k()->main_memory.high	= Kmem::get_mem_max();
  Kip::k()->reserved0.low	= Kmem::kcode_start();
  Kip::k()->reserved0.high	= Kmem::kcode_end();
  Kip::k()->semi_reserved.low	= Boot_info::mbi_virt()->mem_lower << 10;
  Kip::k()->semi_reserved.high	= 1 << 20;
  Kip::k()->sched_granularity	= Config::scheduler_granularity;
  Kip::k()->offset_version_strings = 0xe;
}

IMPLEMENT static inline FIASCO_INIT
void Kip_init::setup_kmem_region (Address, Address)
{
  Kip::k()->reserved1.low	= Kmem::kmem_base();
  Kip::k()->reserved1.high	= Kmem::get_mem_max();
}
