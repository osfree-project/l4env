INTERFACE [arm && realview]:

enum {
  Cache_flush_area = 0,
};

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && realview && mpcore]:
static void map_hw2(void *pd)
{map_1mb(pd, Mem_layout::Mpcore_scu_map_base, Mem_layout::Mpcore_scu_phys_base, false, false); }

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && realview && !mpcore]:
static void map_hw2(void * /*pd*/)
{}

//-----------------------------------------------------------------------------
IMPLEMENTATION [arm && realview]:

#include "mem_layout.h"

void
map_hw(void *pd)
{
  // map devices
  map_1mb(pd, Mem_layout::Devices_map_base, Mem_layout::Devices_phys_base, false, false);
  map_hw2(pd);
}
