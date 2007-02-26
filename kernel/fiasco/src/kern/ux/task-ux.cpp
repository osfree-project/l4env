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
      mem_map (sigma0_task,                         // from: space
	       Kmem::virt_to_phys ((const void*)
		 (Mem_layout::Tbuf_status_page)),   // from: address
	       Config::PAGE_SHIFT,                  // from: size
	       true, 0,                             // write, map
	       nonull_static_cast<Space*>(this),    // to: space
	       Mem_layout::Tbuf_ustatus_page,       // to: address
	       Config::PAGE_SHIFT,                  // to: size
	       0, L4_fpage::Cached);                // to: offset
      for (Address size=0; size<Jdb_tbuf::size(); size+=Config::PAGE_SIZE)
	{
	  mem_map (sigma0_task,                            // from: space
		   Kmem::virt_to_phys ((const void*)
		     (Mem_layout::Tbuf_buffer_area+size)), // from: address
		   Config::PAGE_SHIFT,                     // from: size
		   true, 0,                                // write, map
		   nonull_static_cast<Space*>(this),	   // to: space
		   Mem_layout::Tbuf_ubuffer_area+size,     // to: adddress
		   Config::PAGE_SHIFT,                     // to: size
		   0, L4_fpage::Cached);                   // to: offset
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
	   Mem_layout::Utcb_ptr_frame,		// from: address
	   Config::PAGE_SHIFT,			// from: size
	   true, 0,				// write, grant
	   nonull_static_cast<Space*>(this),    // to: space
	   Mem_layout::Utcb_ptr_page,		// to: address
	   Config::PAGE_SHIFT,			// to: size
	   0, L4_fpage::Cached);		// to: offset
}

// -----------------------------------------------------------------------
IMPLEMENTATION [ux-v2-utcb]:

IMPLEMENT void Task::free_utcb_pagetable() {}
