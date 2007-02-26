INTERFACE:
#include <cstddef>
IMPLEMENTATION:


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
#include "sa_1100.h"

#include "jdb.h"

#include "processor.h"

static int exit_question_active = 0;

static void __attribute__ ((noreturn))
my_pc_reset(void)
{
  
  Sa1100::hw_reg( Sa1100::RSRR_SWR, Sa1100::RSRR );

  for (;;);
}

extern "C" void __attribute__ ((noreturn))
_exit(int)
{
  if(exit_question_active)
    my_pc_reset();
  else
    while(1) { Proc::halt(); Proc::pause(); }
}


static void exit_question()
{
  exit_question_active = 1;

  while(1) {
    puts("\nReturn reboots, \"k\" enters L4 kernel debugger...");

    char c = getchar();
    
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
	my_pc_reset();
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
  atexit(&exit_question);
   
  printf("%s\n", Kip::version_string());

  Kmem_alloc::allocator();


  printf("Interrupts: %i\n", Proc::interrupts());

  
  // disallow all interrupts before we selectively enable them 
  //  pic_disable_all();
  
  // create kernel thread
  static Kernel_thread *kernel = new (&Config::kernel_id) Kernel_thread;
  nil_thread = kernel_thread = kernel; // fill public variable

  //  unsigned dummy;

  puts("switch to thread stack");
  // switch to stack of kernel thread and bootstrap the kernel
  asm volatile
    ("	str sp,%0	        \n"	// save stack pointer in safe register
     "	mov sp,%1	        \n"	// switch stack
     "	mov r0,%2	        \n"	// push "this" pointer
     "	bl call_bootstrap     \n"
     :	"=m" (boot_stack)
     :	"r" (kernel->init_stack()), "r" (kernel));


}
