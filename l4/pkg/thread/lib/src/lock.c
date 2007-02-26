/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/src/lock.c
 * \brief  User thread lock/unlock
 *
 * \date   04/21/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4/L4Env includes */
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* library includes */
#include <l4/thread/thread.h>
#include "__tcb.h"

/*****************************************************************************
 *** l4thread user API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Lock thread, this avoids manipulations by other threads, 
 *         especially that the current thread gets killed by someone else
 * 
 * \param  thread        Thread id
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL invalid thread id
 */
/*****************************************************************************/ 
int
l4thread_lock(l4thread_t thread)
{
  /* lock TCB */
  if (l4th_tcb_get_locked(thread) == NULL)
    return -L4_EINVAL;
  else
    return 0;
}

/*****************************************************************************/
/**
 * \brief  Unlock thread
 * 
 * \param  thread        Thread id
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL invalid thread id
 */
/*****************************************************************************/ 
int
l4thread_unlock(l4thread_t thread)
{
  l4th_tcb_t * tcb;

  /* get TCB */
  tcb = l4th_tcb_get(thread);
  if (tcb == NULL)
    return -L4_EINVAL;

  /* unlock */
  l4th_tcb_unlock(tcb);

  return 0;
}

/*****************************************************************************/
/**
 * \brief  Lock current thread, this avoids manipulations by other threads, 
 *         especially that the current thread gets killed by someone else
 * 
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL  current thread not found in thread table
 */
/*****************************************************************************/ 
int
l4thread_lock_myself(void)
{
  /* get current TCB locked */
  if (l4th_tcb_get_current_locked() == NULL)
    {
      LOG_Error("l4thread: current thread not found in thread table!");
      return -L4_EINVAL;
    }
  else
    return 0;
}

/*****************************************************************************/
/**
 * \brief  Unlock current thread
 *	
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL  current thread not found in thread table
 */
/*****************************************************************************/ 
int
l4thread_unlock_myself(void)
{
  l4th_tcb_t * tcb;

  /* get TCB */
  tcb = l4th_tcb_get_current();
  if (tcb == NULL)
    return -L4_EINVAL;

  /* unlock */
  l4th_tcb_unlock(tcb);

  return 0;
}
