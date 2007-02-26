/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/src/prio_l4.c
 * \brief  Architecture dependend prio handling, native L4 version
 *
 * \date   09/04/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * \todo Right now the L4env has no global idea of priorities, applications
 *       use absolute L4 priorities. Fix this?!
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4/L4env includes */
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>

/* lib includes */
#include "__prio.h"
#include "__tcb.h"
#include "__debug.h"

/*****************************************************************************
 *** l4thread internal functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Init priority handling, set default priority.
 *
 * \return 0 on success (set default priority), error code otherwise:
 *         - -#L4_EINVAL  invalid thread priority
 */
/*****************************************************************************/ 
int
l4th_prio_init(void)
{
  l4_sched_param_t p;
  l4_threadid_t foo = L4_INVALID_ID;
  
  if (l4thread_default_prio == L4THREAD_DEFAULT_PRIO)
    {
      /* use priority of current thread as default priority */
      l4_thread_schedule(l4_myself(),L4_INVALID_SCHED_PARAM,&foo,&foo,&p);
      if (l4_is_invalid_sched_param(p))
	{
	  Error("l4thread: failed to get default priority!");
	  return -L4_EINVAL;
	}
      l4thread_default_prio = p.sp.prio;
    }

  LOGdL(DEBUG_PRIO_INIT,"default prio = %d",l4thread_default_prio);

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief Get priority.
 * 
 * \param  tcb           Thread control block.
 *
 * \return Priority of thread (>= 0), error code otherwise (< 0)
 *         - -#L4_EINVAL invalid thread id
 *
 * Return the L4 priority of thread \a tcb.
 */
/*****************************************************************************/ 
l4_prio_t
l4th_get_prio(l4th_tcb_t * tcb)
{
#if L4THREAD_PRIO_CALL_SCHEDULE
  int do_l4_schedule = 1;
#else
  int do_l4_schedule = 0;
#endif
  l4_sched_param_t p;
  l4_threadid_t foo = L4_INVALID_ID;

  if ((tcb->prio == L4THREAD_DEFAULT_PRIO) || (do_l4_schedule))
    {
      /* get priority */
      l4_thread_schedule(tcb->l4_id,L4_INVALID_SCHED_PARAM, 
			 &foo,&foo,&p);
      
      if (l4_is_invalid_sched_param(p))
	{
	  /* Whats that? */
	  Error("l4thread: failed to get priority!");
	  return -L4_EINVAL;
	}
      
      /* set priority in tcb */
      tcb->prio = p.sp.prio;
    }

  /* return prio */
  return tcb->prio;
}

/*****************************************************************************/
/**
 * \brief  Set priority.
 * 
 * \param  tcb           Thread control block.
 * \param  prio          New priority.
 *
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL Invalid thread id or priority.
 *
 * Set the priority of thread \a tcb to \a prio.
 */
/*****************************************************************************/ 
int
l4th_set_prio(l4th_tcb_t * tcb, l4_prio_t prio)
{
  l4_sched_param_t p;
  l4_threadid_t foo;
  
  /* get old schedule parameters */
  foo = L4_INVALID_ID;
  l4_thread_schedule(tcb->l4_id,L4_INVALID_SCHED_PARAM,&foo,&foo,&p);

  /* set new priority */
  if (!l4_is_invalid_sched_param(p))
    {
      /* set new prio */
      p.sp.prio = prio;
      p.sp.state = 0;
      foo = L4_INVALID_ID;
      l4_thread_schedule(tcb->l4_id,p,&foo,&foo,&p);
    }

  /* done */
  if (!l4_is_invalid_sched_param(p))
    {
      /* set priority succeeded */
      tcb->prio = prio;
      return 0;
    }
  else
    /* error */
    return -L4_EINVAL;
}
    
