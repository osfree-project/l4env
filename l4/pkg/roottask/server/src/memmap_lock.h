#ifndef MEMMAP_LOCK_H
#define MEMMAP_LOCK_H

#include <stdlib.h>
#include <l4/util/atomic.h>

#ifndef USE_OSKIT
#include <l4/env_support/panic.h>
#endif

extern l4_uint32_t memmap_lock;

static inline void
enter_memmap_functions(l4_uint32_t me, l4_threadid_t locker)
{
  if (l4util_xchg32(&memmap_lock, me) != -1)
    {
      int error;
      l4_umword_t d;
      l4_msgdope_t result;
      if ((error = l4_ipc_receive(locker, L4_IPC_SHORT_MSG, &d, &d,
				  L4_IPC_NEVER, &result)) ||
          (error = l4_ipc_send   (locker, L4_IPC_SHORT_MSG, 0, 0,
				  L4_IPC_NEVER, &result)))
	panic("Error 0x%02x woken up\n", error);
    }
}

static inline void
leave_memmap_functions(l4_uint32_t me, l4_threadid_t candidate)
{
  if (!l4util_cmpxchg32(&memmap_lock, me, -1))
    {
      int error;
      l4_umword_t d;
      l4_msgdope_t result;
      if ((error = l4_ipc_call(candidate,
			       L4_IPC_SHORT_MSG, 0, 0,
			       L4_IPC_SHORT_MSG, &d, &d,
			       L4_IPC_NEVER, &result)))
	panic("Error 0x%02x waking up\n", error);
    }
}

#endif /* ! MEMMAP_LOCK_H */
