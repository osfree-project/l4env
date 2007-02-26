/* $Id$ */
/*****************************************************************************/
/**
 * \file    l4util/include/ARCH-x86/lock_wq.h
 * \brief   Better lock implementation (in comparison to lock.h). In the
 *          case of contention we are going into sleep and wait for the
 *          current locker to be woken up. Nevertheless this code has
 *          some limitations considering different thread priorities.
 *          
 *          This code is x86-dependent since we assume 32-bit addresses
 *          in cmpxchg/xchg.
 *
 * \author  Jork Loeser <hohmuth@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef __L4UTIL_LOCK_WQ_H__
#define __L4UTIL_LOCK_WQ_H__

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/util/atomic.h>

typedef struct l4util_wq_lock_queue_elem_t
{
  volatile struct l4util_wq_lock_queue_elem_t *next, *prev;
  l4_threadid_t   id;
} l4util_wq_lock_queue_elem_t;
	
typedef struct
{
  l4util_wq_lock_queue_elem_t *last;
} l4util_wq_lock_queue_base_t;

static inline int 
l4util_wq_lock_lock(l4util_wq_lock_queue_base_t *queue,
		    l4util_wq_lock_queue_elem_t *q);
static inline int
l4util_wq_lock_unlock(l4util_wq_lock_queue_base_t *queue, 
		      l4util_wq_lock_queue_elem_t *q);
static inline int
l4util_wq_lock_locked(l4util_wq_lock_queue_base_t *queue);

/* Implementation */
inline int
l4util_wq_lock_lock(l4util_wq_lock_queue_base_t *queue, 
		    l4util_wq_lock_queue_elem_t *q)
{
  l4util_wq_lock_queue_elem_t *old;
  l4_msgdope_t result;
  int err;
  l4_umword_t dummy;
  
  q->next = 0;
  q->id   = l4_myself();
  old     = (l4util_wq_lock_queue_elem_t*)
		    l4util_xchg32((l4_umword_t*)(&queue->last),
						(l4_umword_t)q);
  if (old != 0)
    {
      /* already locked */
      old->next = q;
      q->prev   = old;
      if ((err = l4_ipc_receive(old->id, L4_IPC_SHORT_MSG, &dummy, &dummy,
                                L4_IPC_NEVER, &result))!=0)
	return err;
      if ((err = l4_ipc_send   (old->id, L4_IPC_SHORT_MSG, 0, 0,
                                L4_IPC_NEVER, &result))!=0)
	return err;
    }
  return 0;
}

inline int
l4util_wq_lock_unlock(l4util_wq_lock_queue_base_t *queue, 
		      l4util_wq_lock_queue_elem_t *q)
{
  volatile l4util_wq_lock_queue_elem_t *other;
  l4_msgdope_t result;
  int err;
  l4_umword_t dummy;

  other = (l4util_wq_lock_queue_elem_t*)
		  l4util_cmpxchg32_res((l4_umword_t*)(&queue->last), 
				       (l4_umword_t)q,
				       (l4_umword_t)NULL);
  if (other == q)
    {
      /* nobody wants the lock */
    }
  else
    {
      /* someone wants the lock */
      while(q->next != other)
	{
	  /* 2 possibilities:
	     - other is next, but didnt sign, give it the time
	     - other is not next, find the next by backward iteration */
	  if (other->prev == NULL)
	    {
	      /* - other didnt sign its prev, give it the time to do this */
	      l4_thread_switch(other->id);
	    }
	  else if (other->prev!=q)
	    {
	      /* 2 poss:
		 - if other is next it might be signed up to now 
		   (other->prev == q)
		   - other is not something else then next (its not NULL,
		     we know this), go backward */
	      other = other->prev;
	    }
	}
      /* now we have the next in other */
      /* send an ipc, timeout never */
      if((err = l4_ipc_call(q->next->id, 
                            L4_IPC_SHORT_MSG, 0,0,
                            L4_IPC_SHORT_MSG, &dummy, &dummy,
                            L4_IPC_NEVER,
		      	    &result))!=0)
	return err;
    }
  return 0;
}

inline int
l4util_wq_lock_locked(l4util_wq_lock_queue_base_t *queue)
{
  return queue->last!=NULL;
}

#endif
