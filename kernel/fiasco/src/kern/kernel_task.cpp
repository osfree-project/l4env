INTERFACE:

#include "task.h"

class Kernel_thread;

class Kernel_task : public Task
{
  friend class Kernel_thread;
};

IMPLEMENTATION[!arm]:

#include "config.h"
#include "kmem.h"

PRIVATE inline
Kernel_task::Kernel_task()
  : Task(Config::kernel_taskno, 
	 Config::kernel_taskno, 
	 Kmem::kdir) 
{}


IMPLEMENTATION:

PUBLIC static Task* 
Kernel_task::kernel_task () 
{
  static Kernel_task task;
  return &task;
}

PUBLIC static inline 
void
Kernel_task::init ()
{
  // Make sure the kernel task is initialized.
  kernel_task();
}
