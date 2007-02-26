/* $Id$ */
/*****************************************************************************/
/**
 * \file   semaphore/include/ARCH-x86/generic.h
 * \brief  Generic semaphore implementation, x86 version
 *
 * \date   11/13/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * This file provides the user programming interface for the L4 semaphore 
 * implementation. 
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
#ifndef _L4_SEMAPHORE_GENERIC_H
#define _L4_SEMAPHORE_GENERIC_H

/* Standard includes */
#include <stdio.h>

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>
#include <l4/util/atomic.h>
#include <l4/env/cdefs.h>
#include <l4/thread/thread.h>

/*****************************************************************************
 *** implementation
 *****************************************************************************/

/*****************************************************************************
 * decrement semaphore counter, block if semaphore locked
 *****************************************************************************/
volatile L4_INLINE void 
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
  while (!cmpxchg32((l4_uint32_t *)&sem->counter,old,tmp));

  if (tmp < 0)
    {
      /* we did not get the semaphore, block */
      ret = l4_i386_ipc_call(l4semaphore_thread_l4_id,
			     L4_IPC_SHORT_MSG,L4SEMAPHORE_BLOCK,
			     (l4_umword_t)sem,
			     L4_IPC_SHORT_MSG,&dummy,&dummy,
			     L4_IPC_NEVER,&result);
      if (ret != 0)
	printf("L4semaphore: block IPC failed (0x%08x)!\n",ret);
    }
}


/*****************************************************************************
 * decrement semaphore counter, return error if semaphore locked
 *****************************************************************************/
volatile L4_INLINE int
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
  while (!cmpxchg32((l4_uint32_t *)&sem->counter,old,tmp));

  /* decremented semaphore counter */
  return 1;
}

/*****************************************************************************
 * decrement semaphore counter, block for a given time if semaphore locked
 *****************************************************************************/
volatile L4_INLINE int
l4semaphore_down_timed(l4semaphore_t * sem, unsigned time)
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
  while (!cmpxchg32((l4_uint32_t *)&sem->counter,old,tmp));

  if (tmp < 0)
    {
      int e, m;
      
      micros2l4to(time*1000, &e, &m);
      /* we did not get the semaphore, block */
      ret = l4_i386_ipc_call(l4semaphore_thread_l4_id,
			     L4_IPC_SHORT_MSG,L4SEMAPHORE_BLOCK,
			     (l4_umword_t)sem,
			     L4_IPC_SHORT_MSG,&dummy,&dummy,
			     L4_IPC_TIMEOUT(0, 0, m, e, 0, 0),&result);
      if (ret != 0)
	{
          /* we had a timeout, do semaphore_up to compensate */
          l4semaphore_up(sem);
	  
	  return 1;
	}
    }
  return 0;
}

/*****************************************************************************
 * increment semaphore counter, wakeup first thread waiting
 *****************************************************************************/
volatile extern inline void
l4semaphore_up(l4semaphore_t * sem)
{
  int old,tmp,ret;
  l4_msgdope_t result;
#if !(SEMAPHORE_SEND_ONLY_IPC)
  l4_umword_t dummy;
#endif

  /* increment semaphore counter */
  do 
    {
      old = sem->counter;
      tmp = old + 1;
    }
  /* retry if someone else also modified the counter */
  while (!cmpxchg32((l4_uint32_t *)&sem->counter,old,tmp));

  if (tmp <= 0)
    {
      /* send message to semaphore thread */
#if L4SEMAPHORE_SEND_ONLY_IPC
      ret = l4_i386_ipc_send(l4semaphore_thread_l4_id,L4_IPC_SHORT_MSG,
			     L4SEMAPHORE_RELEASE,(l4_umword_t)sem,
			     L4_IPC_NEVER,&result);
#else
      ret = l4_i386_ipc_call(l4semaphore_thread_l4_id,L4_IPC_SHORT_MSG,
			     L4SEMAPHORE_RELEASE,(l4_umword_t)sem,
			     L4_IPC_SHORT_MSG,&dummy,&dummy,
			     L4_IPC_NEVER,&result);
#endif
      if (ret != 0)
	printf("L4semaphore: wakeup IPC failed (0x%08x)!\n",ret);
      
    }
}

#endif /* !_L4_SEMAPHORE_GENERIC_H */
