IMPLEMENTATION [arm]:

#include "boot_info.h"
#include "config.h"
#include "irq_alloc.h"

IMPLEMENT inline
void
Kernel_thread::free_initcall_section()
{
  //memset( &_initcall_start, 0, &_initcall_end - &_initcall_start );
}

IMPLEMENT FIASCO_INIT
void
Kernel_thread::bootstrap_arch()
{ Proc::sti(); }

IMPLEMENT
void
Kernel_thread::arch_exit()
{
  exit(0);
}
