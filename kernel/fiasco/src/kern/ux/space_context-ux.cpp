INTERFACE:

#include <sys/types.h>		// for pid_t
#include "kmem.h"
#include "linker_syms.h"

EXTENSION class Space_context
{
public:
 ~Space_context();
  pid_t pid() const;

private:
  static const unsigned long pid_index
    = (Kmem::_unused4_1_addr >> PDESHIFT) & PDEMASK;
};

IMPLEMENTATION[ux]:

#include <cstring>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include "config.h"
#include "cpu_lock.h"
#include "emulation.h"
#include "lock_guard.h"

IMPLEMENT
Space_context::~Space_context()
{
  Lock_guard<Cpu_lock> guard (&cpu_lock);

  pid_t hostpid = pid();
  ptrace (PTRACE_KILL, hostpid, NULL, NULL);

  while (waitpid (hostpid, NULL, 0) != hostpid)
    ;
}

IMPLEMENT inline NEEDS ["kmem.h", "emulation.h"]
Space_context *
Space_context::current()
{
  return reinterpret_cast<Space_context*>(Kmem::phys_to_virt (Emulation::get_pdir_addr()));
}

IMPLEMENT inline NEEDS ["kmem.h", "emulation.h"]
void
Space_context::make_current()
{
  Emulation::set_pdir_addr (Kmem::virt_to_phys (this));
}

IMPLEMENT inline
pid_t
Space_context::pid() const			// returns host pid number
{
  return _dir[pid_index] >> 8;
}

IMPLEMENT inline
void
Space_context::switchin_context()
{
  if (Space_context::current() != this)
    make_current();
}
