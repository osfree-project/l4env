
IMPLEMENTATION[ia32,amd64]:

#include "apic.h"
#include "cmdline.h"
#include "config.h"
#include "cpu.h"
#include "irq_alloc.h"
#include "mem_layout.h"
#include "pic.h"
#include "profile.h"
#include "trap_state.h"
#include "watchdog.h"

IMPLEMENT inline NEEDS["mem_layout.h"]
void
Kernel_thread::free_initcall_section()
{
  // just fill up with invalid opcodes
  for (unsigned short *i = (unsigned short *) &Mem_layout::initcall_start;   
                       i < (unsigned short *) &Mem_layout::initcall_end; i++)
    *i = 0x0b0f;	// UD2 opcode
}

IMPLEMENT FIASCO_INIT
void
Kernel_thread::bootstrap_arch()
{
  // 
  // install our slow trap handler
  //
  nested_trap_handler      = Trap_state::base_handler;
  Trap_state::base_handler = thread_handle_trap;

  //
  // initialize interrupts
  //
  Irq_alloc::lookup(2)->alloc(this, false);	// reserve cascade irq  
  Pic::enable(2);			// allow cascaded irqs

  // initialize the profiling timer
  bool user_irq0 = strstr (Cmdline::cmdline(), "irq0");

  if (Config::scheduler_mode == Config::SCHED_PIT && user_irq0)
    panic("option -irq0 not possible since irq 0 is used for scheduling");

  if (Config::profiling)
    {
      if (user_irq0)
    	panic("options -profile and -irq0 don't mix");
      if (Config::scheduler_mode == Config::SCHED_PIT)
	panic("option -profile' not available since PIT is used as "
              "source for timer tick");

      Irq_alloc::lookup(0)->alloc(this, false);
      Profile::init();
      if (strstr (Cmdline::cmdline(), " -profstart"))
        Profile::start();
    }
  else
    {
      if (! user_irq0 && ! Config::scheduler_mode == Config::SCHED_PIT)
	Irq_alloc::lookup(0)->alloc(this, false); // reserve irq0 even though
    }
}
