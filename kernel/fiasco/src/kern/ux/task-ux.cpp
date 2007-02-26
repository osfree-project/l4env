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
  *(_dir.lookup(Mem_layout::Pid_index)) = Hostproc::create (new_number) << 8;
}

IMPLEMENT
Task::~Task()
{
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
  mem_map (sigma0_space,			// from: space
	   Mem_layout::Utcb_ptr_frame,		// from: address
	   Config::PAGE_SHIFT,			// from: size
	   true, 0,				// write, grant
	   nonull_static_cast<Space*>(this),	// to: space
	   Mem_layout::Utcb_ptr_page,		// to: address
	   Config::PAGE_SHIFT,			// to: size
	   0);					// to: offset
}

// -----------------------------------------------------------------------
IMPLEMENTATION [ux-v2-utcb]:

IMPLEMENT void Task::free_utcb_pagetable() {}
