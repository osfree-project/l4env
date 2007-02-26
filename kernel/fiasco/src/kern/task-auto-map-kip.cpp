IMPLEMENTATION[auto-map-kip]:

#include "config.h"
#include "globals.h"
#include "kmem.h"
#include "l4_types.h"
#include "map_util.h"

IMPLEMENT 
Task::Task( Task_num num )
  : Space(num)
{
  if(space() == Config::sigma0_taskno )
    return;

  mem_fpage_map( sigma0, L4_fpage( 0, 0, Config::PAGE_SHIFT, 
				   Kmem::virt_to_phys(Kmem::info()) ),
		 nonull_static_cast<Space*>(this), 
		 L4_fpage( 0, 0, Config::PAGE_SHIFT, Config::AUTO_MAP_KIP_ADDRESS ),
		 0 );
	    
}
