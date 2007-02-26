
IMPLEMENTATION[std-v2x0]:

#include "task.h"

IMPLEMENT
void
Kernel_thread::init_workload()
{
  //
  // allow the boot task to create more tasks
  //
  for (unsigned i = Config::boot_taskno + 1; i < Space_index::max_space_number; i++)
    check(Space_index(i).set_chief(space_index(), Space_index(Config::boot_taskno)));

  //
  // create sigma0
  //

  // sigma0's chief is the boot task
  Space_index(Config::sigma0_id.task()).set_chief(space_index(), Space_index(Config::sigma0_id.chief()));

  sigma0        = new Task(Config::sigma0_id.task());
  sigma0_thread = new (Config::sigma0_id) Thread (sigma0, Config::sigma0_id,
						  Config::sigma0_prio, 
						  Config::sigma0_mcp);
  
  sigma0_thread->initialize (Kmem::info()->sigma0_eip, 0, 0, 0);

  //
  // create the boot task
  //

  // the boot task's chief is the kernel
  Space_index(Config::boot_id.task()).set_chief(space_index(), Space_index(Config::boot_id.chief()));

  Space *boot         = new Task(Config::boot_id.task());
  Thread *boot_thread = new (Config::boot_id) Thread (boot, Config::boot_id,
						      Config::boot_prio, 
						      Config::boot_mcp);

  boot_thread->initialize(Kmem::info()->root_eip,
			  Kmem::info()->root_esp,
			  sigma0_thread, 0);
}
