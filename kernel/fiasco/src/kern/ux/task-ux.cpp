IMPLEMENTATION [ux]:

#include <sys/ptrace.h>
#include <sys/wait.h>

#include "cpu_lock.h"
#include "hostproc.h"
#include "lock_guard.h"

IMPLEMENT
void
Task::host_init (unsigned new_number)
{
  mem_space()->set_pid (Hostproc::create (new_number));
}

/** Map tracebuffer into each userland task for easy access. */
IMPLEMENT
void
Task::map_tbuf ()
{
  if (id() != Config::sigma0_taskno)
    {
      mem_map (sigma0_task, 
	  L4_fpage(0, 1, Config::PAGE_SHIFT, 
	    Kmem::virt_to_phys ((const void*)(Mem_layout::Tbuf_status_page))),
	  nonull_static_cast<Space*>(this),    // to: space
	  L4_fpage(0, 0, Config::PAGE_SHIFT,
	    Mem_layout::Tbuf_ustatus_page, L4_fpage::Cached), 0);

      for (Address size=0; size<Jdb_tbuf::size(); size+=Config::PAGE_SIZE)
	{
	  mem_map (sigma0_task,                            // from: space
	      L4_fpage(0, 1, Config::PAGE_SHIFT,
		Kmem::virt_to_phys ((const void*)
		  (Mem_layout::Tbuf_buffer_area+size))),
	      nonull_static_cast<Space*>(this),	   // to: space
	      L4_fpage(0, 0, Config::PAGE_SHIFT, 
		Mem_layout::Tbuf_ubuffer_area+size, L4_fpage::Cached), 0);
	}
    }
}

IMPLEMENT
Task::~Task()
{
  if (id() == Config::kernel_taskno)
    {
      reset_dirty();		// Must not deallocate kernel pagetable.
      return;			// No need to kill kernel process.
    }

  cleanup();

  Lock_guard<Cpu_lock> guard (&cpu_lock);

  pid_t hostpid = pid();
  ptrace (PTRACE_KILL, hostpid, NULL, NULL);

  while (waitpid (hostpid, NULL, 0) != hostpid)
    ;
}

IMPLEMENTATION [ux-utcb]:

#include "map_util.h"
#include "mem_layout.h"

IMPLEMENT
void
Task::map_utcb_ptr_page()
{
  mem_map (sigma0_task,			// from: space
      L4_fpage(0, 1, Config::PAGE_SHIFT, Mem_layout::Utcb_ptr_frame),
      nonull_static_cast<Space*>(this),    // to: space
      L4_fpage(0, 0, Config::PAGE_SHIFT, Mem_layout::Utcb_ptr_page,
	L4_fpage::Cached),0);		// to: offset
}

// -----------------------------------------------------------------------
IMPLEMENTATION [ux-v2-utcb]:

IMPLEMENT void Task::free_utcb_pagetable() {}
