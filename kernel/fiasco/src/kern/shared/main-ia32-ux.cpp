/*
 * Fiasco ia32 / UX
 * Shared main startup/shutdown code
 */

INTERFACE[ia32,ux]:

#include "initcalls.h"

class Kernel_thread;


IMPLEMENTATION[ia32,ux]:

#include <cstdio>
#include "config.h"
#include "cpu.h"
#include "div32.h"
#include "globals.h"
#include "kernel_thread.h"
#include "processor.h"

FIASCO_INIT
int
main (void)
{
  unsigned dummy;

  // caution: no stack variables in this function because we're going
  // to change the stack pointer!

  printf ("CPU: %s (%X:%X:%X:%X) Model: %s at %llu MHz\n\n",
           Cpu::vendor_str(), Cpu::family(), Cpu::model(),
           Cpu::stepping(), Cpu::brand(), Cpu::model_str(),
           div32(Cpu::frequency(), 1000000));

  Cpu::show_cache_tlb_info("");

  printf ("\nFreeing init code/data: %lu bytes (%lu pages)\n\n",
          (Address)(&Mem_layout::initcall_end - &Mem_layout::initcall_start),
          (Address)(&Mem_layout::initcall_end - &Mem_layout::initcall_start 
	     >> Config::PAGE_SHIFT));

  // Perform architecture specific initialization
  main_arch();

  // all initializations done, go start the system
  Proc::sti();

  // create kernel thread
  static Kernel_thread *kernel = new (Config::kernel_id) Kernel_thread;

  // switch to stack of kernel thread and bootstrap the kernel
  asm volatile
    ("	movl %%esp, %0		\n\t"	// save stack pointer in safe variable
     "	movl %%esi, %%esp	\n\t"	// switch stack
     "	call call_bootstrap	\n\t"	// bootstrap kernel thread
     :	"=m" (boot_stack),
        "=a" (dummy), "=c" (dummy), "=d" (dummy)
     :	"a"(kernel), "S" (kernel->init_stack()));
  
}
