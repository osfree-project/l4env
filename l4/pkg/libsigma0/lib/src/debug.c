
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/sigma0/sigma0.h>

void
l4sigma0_debug_dump(l4_threadid_t pager)
{
  l4_msgdope_t result;
  l4_umword_t d;

  l4_ipc_call(pager,
              L4_IPC_SHORT_MSG, SIGMA0_REQ_DEBUG_DUMP, 0,
              0, &d, &d,
              L4_IPC_NEVER, &result);
}
