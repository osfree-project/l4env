IMPLEMENTATION[arm]:

#include "config.h"
#include "kmem_space.h"

PRIVATE inline
Kernel_task::Kernel_task()
  : Task(Config::kernel_taskno, 
	 Config::kernel_taskno, 
	 Kmem_space::kdir()) 
{}

