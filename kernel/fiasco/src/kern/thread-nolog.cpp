IMPLEMENTATION [nolog]:

#include <cstring>


// 
// IPC logging
// 

// called from interrupt gate
PUBLIC inline
unsigned
Thread::sys_ipc_log(Syscall_frame *)
{
  return 0;
}


// 
// IPC tracing
// 

PUBLIC inline
unsigned
Thread::sys_ipc_trace(Syscall_frame *)
{
  return 0;
}


// 
// Page-fault logging
// 

static inline
void 
Thread::page_fault_log(Address, unsigned, unsigned)
{
}

PUBLIC static inline
int
Thread::log_page_fault()
{
  return 0;
}

/** L4 system call fpage_unmap.  */
PUBLIC inline
unsigned
Thread::sys_fpage_unmap_log(Syscall_frame *)
{
  return 0;
}

