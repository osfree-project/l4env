IMPLEMENTATION:

#include "config.h"
#include "initcalls.h"
#include "mem_layout.h"
#include "task.h"
#include "thread.h"
#include "types.h"
#include "ram_quota.h"
#include "space_index.h"

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
    check (Space_index (i).set_chief
	   (space_index(), Space_index (Config::boot_taskno)));

  //
  // create sigma0
  //

  // sigma0's chief is the boot task
  Space_index (Config::sigma0_id.task()).set_chief
    (space_index(), Space_index (0));

  sigma0_task = Task::create(Ram_quota::root, Config::sigma0_id.task());
  assert(sigma0_task);

  check (sigma0_task->initialize());
  sigma0_space = sigma0_task->mem_space();

  sigma0_thread = Thread::create(sigma0_task, Config::sigma0_id,
      Config::sigma0_prio, Config::sigma0_mcp);
    
  assert (sigma0_thread);

  Address sp = init_workload_s0_stack();
  sigma0_thread->initialize (Kip::k()->sigma0_ip, sp, 0, 0, 0);

  sigma0_thread->thread_lock()->clear();
  
  //
  // create the boot task
  //

  // the boot task's chief is the kernel
  Space_index (Config::boot_id.task()).set_chief
    (space_index(), Space_index (0));
  
  Task *boot_task = Task::create(Ram_quota::root, Config::boot_id.task());
  assert(boot_task);

  check (boot_task->initialize());
  sigma0_thread->setup_task_caps (boot_task, 0, 0);

  Thread *boot_thread = Thread::create(boot_task, Config::boot_id,
      Config::boot_prio, Config::boot_mcp);
  
  assert (boot_thread);

  boot_thread->initialize(Kip::k()->root_ip,
      Kip::k()->root_sp,
      sigma0_thread, 0, 0);

  boot_thread->thread_lock()->clear();
}

IMPLEMENTATION [ia32,amd64]:

PRIVATE inline
Address
Kernel_thread::init_workload_s0_stack()
{
  // push address of kernel info page to sigma0's stack
  Address sp = Kip::k()->sigma0_sp - sizeof(Mword);
  *Mem_layout::boot_data (reinterpret_cast<Address*>(sp))
    = Kmem::virt_to_phys (Kip::k());
  return sp;
}

IMPLEMENTATION [ux,arm]:

PRIVATE inline
Address
Kernel_thread::init_workload_s0_stack()
{ return Kip::k()->sigma0_sp; }
