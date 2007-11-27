/* $Id$ */
/*****************************************************************************/
/**
 * \file   semaphore/include/archindep.h
 * \brief  Shared generic semaphore implementation
 *
 * \date   06/14/2007
 * \author Alexander Boettcher <boettcher@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _L4_SEMAPHORE_ARCH_INDEP_H
#define _L4_SEMAPHORE_ARCH_INDEP_H

#include <l4/util/atomic.h>
#include <l4/sys/ipc.h>

/*****************************************************************************
 * decrement semaphore counter, return error if semaphore locked
 * no assambler version
 *****************************************************************************/
L4_INLINE int
l4semaphore_try_down(l4semaphore_t * sem)
{
  int old,tmp;

  /* try to decrement the semaphore counter */
  do
    {
      old = sem->counter;

      if (old <= 0)
	/* semaphore already locked */
	return 0;

      tmp = old - 1;
    }
  /* retry if someone else also modified the counter */
  while (!l4util_cmpxchg32((volatile l4_uint32_t *)&sem->counter,
			   (l4_uint32_t)old, (l4_uint32_t)tmp));

  /* decremented semaphore counter */
  return 1;
}

/*****************************************************************************
 * decrement semaphore counter, block for a given time if semaphore locked
 *****************************************************************************/
L4_INLINE int
l4semaphore_down_timed(l4semaphore_t * sem, unsigned timeout)
{
  int old,tmp,ret;
  l4_umword_t dummy;
  l4_msgdope_t result;

  /* a timeout of 0 ms mean try it prompt */
  if (timeout == 0)
    return !l4semaphore_try_down(sem);

  /* decrement counter, check result */
  do
    {
      old = sem->counter;
      tmp = old - 1;
    }
  /* retry if someone else also modified the counter */
  while (!l4util_cmpxchg32((volatile l4_uint32_t *)&sem->counter,
			   (l4_uint32_t)old, (l4_uint32_t)tmp));


  if (tmp < 0)
    {
      /* we did not get the semaphore, block */
      ret = l4_ipc_call(l4semaphore_thread_l4_id,
                        L4_IPC_SHORT_MSG, L4SEMAPHORE_BLOCKTIMED,
                        (l4_umword_t)sem,
                        L4_IPC_SHORT_MSG, &dummy, &dummy,
                        l4_timeout(L4_IPC_TIMEOUT_NEVER,
                        l4util_micros2l4to((timeout == ~0U ? ~0U : timeout * 1000))),
                        &result);

      if (ret != 0)
        {
          do
            {
              old = sem->counter;
              tmp = old + 1;
            }
          /* retry if someone else also modified the counter */
          while (!l4util_cmpxchg32((volatile l4_uint32_t *)&sem->counter,
                                   (l4_uint32_t)old, (l4_uint32_t)tmp));

          /* semaphore thread did not receive IPC, nothing to do*/
          if (ret == L4_IPC_SECANCELED || ret == L4_IPC_SETIMEOUT ||
              ret == L4_IPC_SEABORTED)
            return 1;

          /* we had a timeout, notify the semaphore thread to
           * remove us from queue*/
          while (ret != 0)
            ret = l4_ipc_send(l4semaphore_thread_l4_id, L4_IPC_SHORT_MSG,
                              L4SEMAPHORE_RELEASETIMED, (l4_umword_t)sem,
                              L4_IPC_NEVER, &result);

          return 1;
        }
    }
  return 0;
}

#endif /* !_L4_SEMAPHORE_ASM_H */
