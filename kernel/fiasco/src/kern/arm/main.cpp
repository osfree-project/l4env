INTERFACE [arm]:
#include <cstddef>

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "config.h"
#include "globals.h"
#include "kmem_alloc.h"
#include "kip_init.h"
#include "pagetable.h"
#include "kdb_ke.h"
#include "kernel_thread.h"
#include "kernel_console.h"
#include "reset.h"
#include "space.h"
#include "terminate.h"

#include "processor.h"

static int exit_question_active = 0;

extern "C" void __attribute__ ((noreturn))
_exit(int)
{
  if(exit_question_active)
    pc_reset();
  else
    while(1) { Proc::halt(); Proc::pause(); }
}


static void exit_question()
{
  exit_question_active = 1;

  while (1)
    {
      puts("\nReturn reboots, \"k\" enters L4 kernel debugger...");

      char c = Kconsole::console()->getchar();

      if (c == 'k' || c == 'K')
	{
	  kdb_ke("_exit");
	}
      else
	{
	  // it may be better to not call all the destruction stuff
	  // because of unresolved static destructor dependency
	  // problems.
	  // SO just do the reset at this point.
	  puts("\033[1mRebooting...\033[0m");
	  pc_reset();
	  break;
	}
    }
}


int main()
{
  // caution: no stack variables in this function because we're going
  // to change the stack pointer!

  // make some basic initializations, then create and run the kernel
  // thread
  set_exit_question(&exit_question);

  printf("%s\n", Kip::k()->version_string());

  // disallow all interrupts before we selectively enable them
  //  pic_disable_all();

  // create kernel thread
  static Kernel_thread *kernel = new (Config::kernel_id) Kernel_thread;
  Mem_space::kernel_space( current_mem_space() );

  kdb_ke("init");

  // switch to stack of kernel thread and bootstrap the kernel
  asm volatile
    ("	str sp,%0	        \n"	// save stack pointer in safe register
     "	mov sp,%1	        \n"	// switch stack
     "	mov r0,%2	        \n"	// push "this" pointer
     "	bl call_bootstrap     \n"
     :	"=m" (boot_stack)
     :	"r" (kernel->init_stack()), "r" (kernel));
}

