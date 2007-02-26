INTERFACE:

#include "thread.h"

class Kernel_thread : public Thread
{
private:
  /**
   * @brief Frees the memory of the initcall sections.  
   *
   * Virtually initcall sections are freed by not marking them
   * reserved in the KIP. This method just invalidates the contents of
   * the memory, by filling it with some invalid data and may be
   * unmapping it.
   */
  void	free_initcall_section();
  void	bootstrap() 		asm ("call_bootstrap");
  void	bootstrap_arch();
  void	run();

protected:
  void	init_workload();
};

IMPLEMENTATION:

#include <cstdlib>
#include <cstdio>

#include "config.h"
#include "globals.h"
#include "helping_lock.h"
#include "kmem.h"
#include "linker_syms.h"
#include "processor.h"
#include "thread.h"
#include "thread_state.h"
#include "timer.h"
#include "vmem_alloc.h"

// overload allocator -- we cannot allocate by page fault during
// bootstrapping
PUBLIC
void *
Kernel_thread::operator new(size_t s, Threadid id)
{
  // call superclass' allocator
  void *addr = Thread::operator new(s, id);

  // explicitly allocate and enter in page table -- we cannot allocate
  // by page fault during bootstrapping
  
  if (! Vmem_alloc::page_alloc((void*)((Address)addr & Config::PAGE_MASK), 
			       0, Vmem_alloc::ZERO_FILL))
    panic("can't allocate kernel tcb");

  return addr;
}

PUBLIC inline
Kernel_thread::Kernel_thread()
             : Thread (current_space(), Config::kernel_id)
{}

PUBLIC inline
Mword *Kernel_thread::init_stack() 
{
  return kernel_sp; 
}

// the kernel bootstrap routine
IMPLEMENT FIASCO_INIT
void
Kernel_thread::bootstrap()
{
  // Initializations done -- Helping_lock can now use helping lock
  Helping_lock::threading_system_active = true;

  // Set up my own thread control block
  state_change (0, Thread_running);

  ready_next = ready_prev = present_next = present_prev = this;

  // Set up class variables
  for (int i = 0; i < 256; i++)
    prio_next[i] = prio_first[i] = 0;

  prio_next[Config::kernel_prio] = prio_first[Config::kernel_prio] = this;
  prio_highest = Config::kernel_prio;

  // Setup initial timeslice
  set_current_sched (sched());

  Timer::enable();

  bootstrap_arch();

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
  free_initcall_section();		// No initcalls after this function

  // init_workload cannot be an initcall, because it fires up the userland
  // applications which then have access to initcall frames as per kinfo page.
  init_workload();

  while (running) 
    {
      // Interrupts must be enabled, otherwise idling is fatal
      assert (Proc::interrupts());

      idle();

      while (running && ready_next != this)	// are there any other threads ready?
	schedule();
    }

  puts ("\nExiting, wait...");

  // Boost this thread's priority to the maximum. Since this thread tears down
  // all other threads, we want it to run whenever possible to advance as fast
  // as we can.
  ready_dequeue();
  sched()->set_prio(255);
  ready_enqueue();

  kill_all();					// Nuke everything else

  Helping_lock::threading_system_active = false;

  exit (0);					// Commit suicide
}

PUBLIC inline NEEDS["processor.h"]
void
Kernel_thread::idle()
{
  if (Config::hlt_works_ok)
    Proc::halt();  // stop the CPU, waiting for an int
  else
    Proc::pause();
}
