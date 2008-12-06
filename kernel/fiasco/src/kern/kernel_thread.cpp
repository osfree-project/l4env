INTERFACE:

#include "thread.h"

class Kernel_thread : public Thread
{
private:
  /**
   * Frees the memory of the initcall sections.
   *
   * Virtually initcall sections are freed by not marking them
   * reserved in the KIP. This method just invalidates the contents of
   * the memory, by filling it with some invalid data and may be
   * unmapping it.
   */
  void	free_initcall_section();
  void	bootstrap()		asm ("call_bootstrap") FIASCO_FASTCALL;
  void	bootstrap_arch();
  void	run();
  void  arch_exit() __attribute__((noreturn));

protected:
  void	init_workload();
};

IMPLEMENTATION:

#include <cstdlib>
#include <cstdio>

#include "config.h"
#include "delayloop.h"
#include "globals.h"
#include "helping_lock.h"
#include "kernel_task.h"
#include "processor.h"
#include "task.h"
#include "thread.h"
#include "thread_state.h"
#include "timer.h"
#include "vmem_alloc.h"

// overload allocator -- we cannot allocate by page fault during
// bootstrapping
PUBLIC
void *
Kernel_thread::operator new (size_t, L4_uid id)
{
  // call superclass' allocator
  Thread *addr = id_to_tcb (id);
  if (! Vmem_alloc::page_alloc ((void*)((Address)addr & Config::PAGE_MASK),
	  Vmem_alloc::ZERO_FILL))
    panic("can't allocate kernel tcb");

  // explicitly allocate and enter in page table -- we cannot allocate
  // by page fault during bootstrapping


  return addr;
}


PUBLIC
Kernel_thread::Kernel_thread()
  : Thread (Kernel_task::kernel_task(), Config::kernel_id)
{}

PUBLIC inline
Mword *
Kernel_thread::init_stack()
{
  return _kernel_sp;
}

// the kernel bootstrap routine
IMPLEMENT FIASCO_INIT
void
Kernel_thread::bootstrap()
{
  // Initializations done -- Helping_lock can now use helping lock
  Helping_lock::threading_system_active = true;

  state_change (0, Thread_ready);		// Set myself ready
  present_next = present_prev = this;		// Initialize present list

  Timer::init_system_clock();

  // Setup initial timeslice
  set_current_sched (sched());

  Timer::enable();

  // all initializations done, go start the system
  Proc::sti();

  bootstrap_arch();

  printf("Calibrating timer loop... ");
  // Init delay loop, needs working timer interrupt
  if (running)
    Delay::init();
  printf("done.\n");

  run();
}

/**
 * The idle loop
 * NEVER inline this function, because our caller is an initcall
 */
IMPLEMENT FIASCO_NOINLINE FIASCO_NORETURN
void
Kernel_thread::run()
{
  free_initcall_section();

  // No initcalls after this point!

  // initialize cpu time of the idle thread
  Context::init_switch_time();

  // init_workload cannot be an initcall, because it fires up the userland
  // applications which then have access to initcall frames as per kinfo page.
  init_workload();

  while (running)
    idle();

  puts ("\nExiting, wait...");

  // Boost this thread's priority to the maximum. Since this thread tears down
  // all other threads, we want it to run whenever possible to advance as fast
  // as we can.
  ready_dequeue();
  sched()->set_prio(255);
  ready_enqueue();

  kill_all();					// Nuke everything else

  Helping_lock::threading_system_active = false;

  arch_exit();
}

PUBLIC inline NEEDS["processor.h"]
void
Kernel_thread::idle()
{
  if (Config::hlt_works_ok)
    Proc::halt();			// stop the CPU, waiting for an int
  else
    Proc::pause();
}

