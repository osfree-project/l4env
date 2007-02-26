INTERFACE:

EXTENSION class Thread
{
public:
  static bool log_ipc;
  static bool log_ipc_to_buf;
  static bool log_ipc_result;
  static bool log_page_fault;
  static bool log_pf_to_buf;
  static bool log_unmap;
  static bool log_unmap_to_buf;
  static bool trace_ipc;
  static bool slow_ipc;
  static bool cpath_ipc;
};

IMPLEMENTATION [nolog]:

#include <alloca.h>
#include <cstring>

bool Thread::log_ipc          = false;
bool Thread::log_ipc_to_buf   = false;
bool Thread::log_ipc_result   = false;
bool Thread::log_page_fault   = false;
bool Thread::log_pf_to_buf    = false;
bool Thread::log_unmap        = false;
bool Thread::log_unmap_to_buf = false;
bool Thread::trace_ipc        = false;
bool Thread::slow_ipc         = false;
bool Thread::cpath_ipc        = false;

// 
// System-call logging
// 

bool log_system_call = false;

PUBLIC inline NOEXPORT
void 
Thread::syscall_log(Syscall_frame *, unsigned, bool)
{
}

extern "C" void
thread_syscall_log_begin(Thread *, Syscall_frame *, unsigned)
{
}

extern "C" void
thread_syscall_log_end(unsigned, Thread *, Syscall_frame *, unsigned)
{
}

// 
// IPC logging
// 

// called from interrupt gate
PUBLIC
unsigned
Thread::sys_ipc_log(Syscall_frame *)
{
  return 0;
}


// 
// IPC tracing
// 

PUBLIC
unsigned
Thread::sys_ipc_trace(Syscall_frame *)
{
  return 0;
}

static
void
Thread::set_ipc_vector()
{
}


// 
// Page-fault logging
// 

static
void 
Thread::page_fault_log(vm_offset_t, unsigned, unsigned)
{
}

/** L4 system call fpage_unmap.  */
PUBLIC
unsigned
Thread::sys_fpage_unmap_log(Syscall_frame *)
{
  return 0;
}

static
void
Thread::hook_unmap_vector()
{
}

static
void
Thread::restore_unmap_vector()
{
}
