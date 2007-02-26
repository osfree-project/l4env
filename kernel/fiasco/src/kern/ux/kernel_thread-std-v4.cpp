IMPLEMENTATION[std-v4]:

#include "config.h"
#include "initcalls.h"
#include "task.h"
#include "thread.h"
#include "types.h"

IMPLEMENT 
void
Kernel_thread::init_workload()
{
  L4_uid sigma0_thread_id (Kmem::info()->user_base(), 1);
  L4_uid boot_thread_id (Kmem::info()->user_base() + 2, 1);

  //
  // create sigma0
  //
  sigma0 = new Task (L4_fpage (1, 1, 0, 
			       Config::SIGMA0_UTCB_AREA_SIZE, 
			       Config::SIGMA0_UTCB_AREA_BASE),
		     L4_fpage (1, 0, 0,
			       Config::PAGE_SHIFT,
			       Config::SIGMA0_KIP_LOCATION));

  sigma0_thread = new (&sigma0_thread_id) 
    Thread (sigma0, sigma0_thread_id,
	    L4_uid (Config::SIGMA0_UTCB_LOCATION, 0, 0),
	    Config::sigma0_prio, 
	    Config::sigma0_mcp);
  
  sigma0_thread->initialize(Kmem::info()->sigma0_eip, 0, 0, 0);

  //
  // create the boot task
  //
  global_root_task = 
    new Task (L4_fpage (1, 1, 0,
			Config::ROOT_UTCB_AREA_SIZE,
			Config::ROOT_UTCB_AREA_BASE), 
	      L4_fpage (1, 0, 0,
			Config::PAGE_SHIFT,
			Config::ROOT_KIP_LOCATION));

  Thread *boot_thread = new (&boot_thread_id) 
    Thread (global_root_task,
	    boot_thread_id,
	    L4_uid (Config::ROOT_UTCB_LOCATION, 0, 0),
	    Config::boot_prio, 
	    Config::boot_mcp);
  
  boot_thread->initialize(Kmem::info()->root_eip,
			  Kmem::info()->root_esp,
			  sigma0_thread, 0);
}

