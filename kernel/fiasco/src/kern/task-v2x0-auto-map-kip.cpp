IMPLEMENTATION[v2x0-auto-map-kip]:

#include "config.h"
#include "kmem.h"
#include "l4_types.h"
#include "map_util.h"

IMPLEMENT 
Task::Task (Task_num id)
    : Space (id)
{
  if (space() == Config::sigma0_taskno)
    return;

  mem_map (sigma0, 
	   Kmem::virt_to_phys (Kmem::info()),
	   Config::PAGE_SHIFT,
	   0, 0,
	   nonull_static_cast<Space*>(this), 
	   Config::AUTO_MAP_KIP_ADDRESS,
	   Config::PAGE_SHIFT, 0);
}
