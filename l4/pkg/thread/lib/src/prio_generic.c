/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/src/prio_generic.c
 * \brief  Priority handling, generic API functions
 *
 * \date   09/04/2000
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

/* L4/L4env includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>

/* lib includes */
#include <l4/thread/thread.h>
#include "__prio.h"
#include "__tcb.h"

/*****************************************************************************
 *** l4thread user API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Get priority.
 * 
 * \param  thread        Thread id.
 *
 * \return Priority of \a thread (>= 0), error code otherwise (< 0)
 *         - -#L4_EINVAL invalid thread id
 */
/*****************************************************************************/ 
l4_prio_t 
l4thread_get_prio(l4thread_t thread)
{
  l4th_tcb_t * tcb;

  /* get tcb */
  tcb = l4th_tcb_get_active(thread);
  if (tcb == NULL)
    /* invalid thread */
    return -L4_EINVAL;

  /* return priority */
  return l4th_get_prio(tcb);
}

/*****************************************************************************/
/**
 * \brief  Set priority.
 * 
 * \param  thread        Thread id.
 * \param  prio           New priority.
 *
 * \return 0 on success, error code otherwise:
 *         - -#L4_EINVAL Invalid thread id or priority.
 *
 * Set the L4 priority of \a thread to \a prio.
 */
/*****************************************************************************/ 
int 
l4thread_set_prio(l4thread_t thread, l4_prio_t prio)
{
  l4th_tcb_t * tcb;
  int ret;
  
  /* get tcb */
  tcb = l4th_tcb_get_active_locked(thread);
  if (tcb == NULL)
    /* invalid thread */
    return -L4_EINVAL;

  /* set priority */
  ret = l4th_set_prio(tcb,prio);
  l4th_tcb_unlock(tcb);

  /* done */
  return ret;
}
