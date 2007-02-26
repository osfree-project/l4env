/* $Id$ */
/*****************************************************************************/
/**
 * \file   thread/lib/include/__l4.h
 * \brief  L4 specific thread handling
 *
 * \date   04/11/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef _THREAD___L4_H
#define _THREAD___L4_H

/* L4 includes */
#include <l4/env/cdefs.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>

/* library includes */
#include <l4/thread/thread.h>

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

L4_INLINE l4_threadid_t
l4th_l4_to_l4id(l4_threadid_t task, l4thread_t t);

L4_INLINE l4thread_t
l4th_l4_from_l4id(l4_threadid_t id);

L4_INLINE void
l4th_l4_create_thread(l4_threadid_t id, l4_addr_t eip, l4_addr_t esp,
		      l4_threadid_t pager);

L4_INLINE l4_threadid_t
l4th_l4_myself(void);

/*****************************************************************************
 *** implementation
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Get L4 thread id from thread id
 * 
 * \param  task          Task id 
 * \param  t             Thread id
 *	
 * \return L4 thread id
 */
/*****************************************************************************/ 
L4_INLINE l4_threadid_t
l4th_l4_to_l4id(l4_threadid_t task, l4thread_t t)
{
  l4_threadid_t id = task;

  id.id.lthread = t;

  return id;
}

/*****************************************************************************/
/**
 * \brief  Get thread id from L4 thread id
 * 
 * \param  id            L4 thread id
 *	
 * \return thread id
 */
/*****************************************************************************/ 
L4_INLINE l4thread_t
l4th_l4_from_l4id(l4_threadid_t id)
{
  return id.id.lthread;
}

/*****************************************************************************/
/**
 * \brief  Create L4 thread
 * 
 * \param  id            L4 threadid
 * \param  eip           Thread eip
 * \param  esp           Thread esp
 */
/*****************************************************************************/ 
L4_INLINE void
l4th_l4_create_thread(l4_threadid_t id, l4_addr_t eip, l4_addr_t esp,
		      l4_threadid_t pager)
{
  l4_threadid_t my_pager,preempter;
  l4_umword_t dummy;

  /* get preempter/pager */
  preempter = my_pager = L4_INVALID_ID;
  l4_thread_ex_regs(l4_myself(),(l4_umword_t)-1,(l4_umword_t)-1,&preempter,
                    &my_pager,&dummy,&dummy,&dummy);
  
  /* create thread */
  if (l4_is_invalid_id(pager))
    l4_thread_ex_regs(id,eip,esp,&preempter,&my_pager,&dummy,&dummy,&dummy);
  else
    l4_thread_ex_regs(id,eip,esp,&preempter,&pager,&dummy,&dummy,&dummy);
}

/*****************************************************************************/
/**
 * \brief  Ask L4 for my thread id
 *	
 * \return My L4 thread id
 */
/*****************************************************************************/ 
L4_INLINE l4_threadid_t
l4th_l4_myself(void)
{
  return l4_myself();
}

#endif /* !_THREAD___L4_H */
