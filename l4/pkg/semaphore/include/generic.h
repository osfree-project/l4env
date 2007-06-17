/* $Id$ */
/*****************************************************************************/
/**
 * \file   semaphore/include/ARCH-x86/generic.h
 * \brief  Generic semaphore implementation, x86 version
 *
 * \date   11/13/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _L4_SEMAPHORE_GENERIC_H
#define _L4_SEMAPHORE_GENERIC_H

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/util/atomic.h>
#include <l4/env/cdefs.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>

/*****************************************************************************
 *** external functions
 *****************************************************************************/
__BEGIN_DECLS
/* internal use only */
void l4semaphore_restart_up(l4semaphore_t *sem,l4_msgdope_t lastresult);
void l4semaphore_restart_down(l4semaphore_t *sem,l4_msgdope_t lastresult);
__END_DECLS

/*****************************************************************************
 *** implementation
 *****************************************************************************/

/*****************************************************************************
 * increment semaphore counter, wakeup first thread waiting
 *****************************************************************************/
L4_INLINE void
l4semaphore_up(l4semaphore_t * sem)
{
  int old,tmp,ret;
  l4_msgdope_t result;
#if !(L4SEMAPHORE_SEND_ONLY_IPC)
  l4_umword_t dummy;
#endif

  /* increment semaphore counter */
  do
    {
      old = sem->counter;
      tmp = old + 1;
    }
  /* retry if someone else also modified the counter */
  while (!l4util_cmpxchg32((l4_uint32_t *)&sem->counter, old, tmp));

  if (tmp <= 0)
    {
      /* send message to semaphore thread */
#if L4SEMAPHORE_SEND_ONLY_IPC

#if L4SEMAPHORE_RESTART_IPC
      do
        {
          ret = l4_ipc_send(l4semaphore_thread_l4_id, L4_IPC_SHORT_MSG,
                            L4SEMAPHORE_RELEASE, (l4_umword_t)sem,
                            L4_IPC_NEVER, &result);

        }
      while (ret == L4_IPC_SECANCELED);
#else /* !L4SEMAPHORE_RESTART_IPC */
      ret = l4_ipc_send(l4semaphore_thread_l4_id, L4_IPC_SHORT_MSG,
			L4SEMAPHORE_RELEASE, (l4_umword_t)sem,
			L4_IPC_NEVER, &result);
#endif /* L4SEMAPHORE_RESTART_IPC */

#else /* !L4SEMAPHORE_SEND_ONLY_IPC */

#if L4SEMAPHORE_RESTART_IPC
      do
        {
          ret = l4_ipc_call(l4semaphore_thread_l4_id, L4_IPC_SHORT_MSG,
                            L4SEMAPHORE_RELEASE, (l4_umword_t)sem,
                            L4_IPC_SHORT_MSG, &dummy, &dummy,
                            L4_IPC_NEVER, &result);
        }
      while (ret == L4_IPC_SECANCELED);

      while (ret == L4_IPC_RECANCELED)
        {
          ret = l4_ipc_receive(l4semaphore_thread_l4_id, L4_IPC_SHORT_MSG,
                               &dummy, &dummy, L4_IPC_NEVER, &result);
        }
#else /* !L4SEMAPHORE_RESTART_IPC */
      ret = l4_ipc_call(l4semaphore_thread_l4_id, L4_IPC_SHORT_MSG,
			L4SEMAPHORE_RELEASE, (l4_umword_t)sem,
			L4_IPC_SHORT_MSG, &dummy, &dummy,
			L4_IPC_NEVER, &result);
#endif /* L4SEMAPHORE_RESTART_IPC */

#endif /* L4SEMAPHORE_SEND_ONLY_IPC */

      if (ret != 0)
	l4env_perror("L4semaphore: wakeup IPC", ret);

    }
}

/*****************************************************************************
 * decrement semaphore counter, block if semaphore locked
 *****************************************************************************/
L4_INLINE void
l4semaphore_down(l4semaphore_t * sem)
{
  int old,tmp,ret;
  l4_umword_t dummy;
  l4_msgdope_t result;

  /* decrement counter, check result */
  do
    {
      old = sem->counter;
      tmp = old - 1;
    }
  /* retry if someone else also modified the counter */
  while (!l4util_cmpxchg32((l4_uint32_t *)&sem->counter, old, tmp));

  if (tmp < 0)
    {
      /* we did not get the semaphore, block */
#if L4SEMAPHORE_RESTART_IPC
      do
        {
          ret = l4_ipc_call(l4semaphore_thread_l4_id,
                            L4_IPC_SHORT_MSG, L4SEMAPHORE_BLOCK,
                            (l4_umword_t)sem,
                            L4_IPC_SHORT_MSG, &dummy, &dummy,
                            L4_IPC_NEVER, &result);
        }
      while (ret == L4_IPC_SECANCELED);

      while (ret == L4_IPC_RECANCELED)
        {
          ret = l4_ipc_receive(l4semaphore_thread_l4_id,
                               L4_IPC_SHORT_MSG, &dummy, &dummy,
                               L4_IPC_NEVER, &result);
        }
#else
      ret = l4_ipc_call(l4semaphore_thread_l4_id,
			L4_IPC_SHORT_MSG, L4SEMAPHORE_BLOCK,
			(l4_umword_t)sem,
			L4_IPC_SHORT_MSG, &dummy, &dummy,
			L4_IPC_NEVER, &result);
#endif
      if (ret != 0)
	l4env_perror("L4semaphore: block IPC failed", ret);
    }
}
#endif /* !_L4_SEMAPHORE_GENERIC_H */
