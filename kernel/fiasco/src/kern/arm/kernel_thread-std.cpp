IMPLEMENTATION [arm]:

#include "config.h"
#include "initcalls.h"
#include "space_index.h"
#include "task.h"
#include "thread.h"
#include "types.h"

IMPLEMENT 
void
Kernel_thread::init_workload()
{
  //
  // allow the boot task to create more tasks
  //

  for (unsigned i = Config::boot_taskno + 1; 
       i < Space_index::Max_space_number;
       i++)
    {
      check(Space_index(i).set_chief(space_index(), 
				     Space_index(Config::boot_taskno)));
    }

  //
  // create sigma0
  //
  //puts("Kernel_thread::init_workload(): create sigma0 task");

  // sigma0's chief is the boot task
  Space_index(Config::sigma0_id.task()).
    set_chief(space_index(), Space_index(Config::sigma0_id.chief()));

  sigma0_task = new Task(Config::sigma0_id.task());
  sigma0_space = sigma0_task;

  //puts("Kernel_thread::init_workload(): create sigma0 thread");
  sigma0_thread = lookup (Config::sigma0_id);

  {
    Lock_guard <Thread_lock> guard (sigma0_thread->thread_lock());

    new (Config::sigma0_id) Thread (sigma0_task, 
				    Config::sigma0_id, 
				    Config::sigma0_prio, 
				    Config::sigma0_mcp);
  
    //puts("Kernel_thread::init_workload(): initialize sigma0");
    sigma0_thread->initialize(Kip::k()->sigma0_pc, 
			      Kip::k()->sigma0_sp,
			      0, 0);
  }

  //
  // create the boot task
  //

  //puts("Kernel_thread::init_workload(): create root task");

  // the boot task's chief is the kernel
  Space_index(Config::boot_id.task()).
    set_chief(space_index(), Space_index(Config::boot_id.chief()));

  Task *boot = new Task(Config::boot_id.task());
  //puts("Kernel_thread::init_workload(): create root thread");
  //printf("ID: %x.%x\n",Config::boot_id.task(),Config::boot_id.lthread());
  
  Thread *boot_thread = lookup (Config::boot_id);
  {
    Lock_guard <Thread_lock> guard (boot_thread->thread_lock());

    new (Config::boot_id) Thread (boot, Config::boot_id, 
				  Config::boot_prio, 
				  Config::boot_mcp);

    //puts("Kernel_thread::init_workload(): initialize root");

    boot_thread->initialize(Kip::k()->root_pc,
			    Kip::k()->root_sp,
			    sigma0_thread, 0);
  }
}

