/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/src/tcb_l4.c
 * \brief  Thread control block handling, native L4 version
 *
 * \date   09/02/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/atomic.h>

/* private includes */
#include <l4/thread/thread.h>
#include "__tcb.h"

/*****************************************************************************
 *** l4thread internal functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Allocate new tcb.
 * 
 * \retval tcb           Pointer to thread control block
 * 
 * \return 0 on success, error code otherwise:
 *         - -#L4_ENOTHREAD  thread table full
 */
/*****************************************************************************/ 
int
l4th_tcb_allocate(l4th_tcb_t ** tcb)
{
  int i;

  /* search unused tcb in tcb table */
  for (i = 0; i < l4thread_max_threads; i++)
    {
      /* try to set internal thread number to index in tcb table */
      if (l4util_cmpxchg16(&l4th_tcbs[i].state,TCB_UNUSED,TCB_SETUP))
	/* found unused tcb */
	break;
    }

  if (i == l4thread_max_threads)
    {
      /* no tcb found */
      *tcb = NULL;

      return -L4_ENOTHREAD;
    }
  else
    {
      /* found unused tcb */
      l4th_tcbs[i].id = i;
      *tcb = &l4th_tcbs[i];

      return 0;
    }
}

/*****************************************************************************/
/**
 * \brief  Allocate tcb for specified thread.
 * 
 * \param  thread        Thread id
 * \retval tcb           Pointer to thread control block
 *
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL invalid thread id
 *         - -#L4_EUSED  tcb already used
 *
 * Allocate thread control block for thread specified by \a thread.
 */
/*****************************************************************************/ 
int
l4th_tcb_allocate_id(l4thread_t thread, l4th_tcb_t ** tcb)
{
  if ((thread < 0) || (thread >= l4thread_max_threads))
    return -L4_EINVAL;

  /* try to get tcb */
  if (l4util_cmpxchg16(&l4th_tcbs[thread].state,TCB_UNUSED,TCB_SETUP))
    {
      /* tcb unused */
      l4th_tcbs[thread].id = thread;
      *tcb = &l4th_tcbs[thread];

      return 0;
    }
  else
    /* tcb already used */
    return -L4_EUSED;
}

/*****************************************************************************/
/**
 * \brief  Deallocate thread
 * 
 * \param  tcb           Thread control block
 */
/*****************************************************************************/ 
void
l4th_tcb_deallocate(l4th_tcb_t * tcb)
{
  /* nothing to do */
}

/*****************************************************************************/
/**
 * \brief  Release tcb.
 * 
 * \param  tcb           Thread control block.
 */
/*****************************************************************************/ 
void
l4th_tcb_free(l4th_tcb_t * tcb)
{
  /* mark unused */
  tcb->state = TCB_UNUSED;
}
