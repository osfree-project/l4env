/* $Id$ */
/**
 * \file	roottask/server/src/memmap_lock.c
 * \brief	start, stop, kill, etc. applications
 *
 * \date	2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 * These two functions implement a very simplified lock for exclusive access
 * to the __memmap arrays. Because there are only two threads in this server
 * we know who has the lock if it is not free. */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <stdlib.h>
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/util/atomic.h>
#include <l4/env_support/panic.h>

#include "memmap_lock.h"

static l4_uint32_t memmap_lock = -1;

/**
 * Grab the memmap lock.
 * \param me     the own thread ID
 * \param locker the other thread ID*/
void
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

/**
 * Release the memmap lock.
 * \param me     the own thread ID
 * \param locker the other thread ID */
void
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
