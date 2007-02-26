/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/src/thread.c
 * \brief  Setup and various simple API functions.
 *
 * \date   09/02/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/

/* L4env includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/util/atomic.h>

/* private includes */
#include <l4/thread/thread.h>
#include "__thread.h"
#include "__tcb.h"
#include "__stacks.h"
#include "__prio.h"
#include "__debug.h"

/**
 * Thread lib initilialized
 */
static int l4th_initialized = 0;

/*****************************************************************************
 *** l4thread user API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Init L4 thread library.
 * 
 * \return 0 on success, error code (< 0) otherwise
 * 
 * Initialize thread library. 
 */
/*****************************************************************************/ 
int
l4thread_init(void)
{
  int ret;

  /* avoid multiple initializations */
  if (!cmpxchg32(&l4th_initialized,0,1))
    return 0;

#if DEBUG_CONFIG
  INFO("l4thread config:\n");
  DMSG("  max. threads:       %d\n",l4thread_max_threads);
  DMSG("  default stack size: %u\n",l4thread_stack_size);
  DMSG("  max. stack size:    %u\n",l4thread_max_stack);
  DMSG("  default priority:   %d\n",l4thread_default_prio);
#endif

  /* init stacks area */
  ret = l4th_stack_init();
  if (ret < 0)
    return ret;

  /* init tcb table */
  ret = l4th_tcb_init();
  if (ret < 0)
    return ret;

  /* set default priority */
  ret = l4th_prio_init();
  if (ret < 0)
    return ret;

  /* architecture dependent init */
  ret = l4th_init_arch();
  if (ret < 0)
    return ret;

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Return thread id of current thread.
 *
 * \return Thread id of current thread, #L4THREAD_INVALID_ID if not found
 */
/*****************************************************************************/ 
l4thread_t 
l4thread_myself(void)
{
  return l4th_tcb_get_current_id();
}

/*****************************************************************************/
/**
 * \brief  Get L4 id of thread.
 * 
 * \param  thread        Thread id
 *
 * \return L4 id of \a thread, #L4_INVALID_ID if invalid thread id.
 */
/*****************************************************************************/ 
l4_threadid_t 
l4thread_l4_id(l4thread_t thread)
{
  l4th_tcb_t * tcb = l4th_tcb_get(thread);

  if (tcb != NULL)
    /* return L4 id */
    return tcb->l4_id;
  else
    return L4_INVALID_ID;
}
  
/*****************************************************************************/
/**
 * \brief  Get thread id of parent thread.
 * 
 * \return Thread id of parent thread, #L4THREAD_INVALID_ID if parent not 
 *         exists.
 *
 * \todo Check if parent still exists (should we implement something like 
 *       'thread trees').
 */
/*****************************************************************************/ 
l4thread_t 
l4thread_get_parent(void)
{
  l4th_tcb_t * tcb = l4th_tcb_get_current();
  l4thread_t parent = L4THREAD_INVALID_ID;

  if ((tcb == NULL) || (tcb->state != TCB_ACTIVE))
    {
      Error("l4thread: myself not found!");
      return L4THREAD_INVALID_ID;
    }
  
  /* get parent thread id */
  if (tcb->parent != NULL)
    parent = tcb->parent->id;

  /* done */
  return parent;
}
