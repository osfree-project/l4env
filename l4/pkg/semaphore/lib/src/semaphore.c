/* $Id$ */
/*****************************************************************************/
/**
 * \file   semaphore/lib/src/semaphore.c
 * \brief  Semaphore thread implementation
 *
 * \date   11/16/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Semaphore thread implementation.
 * 
 * \todo Find another way to pass the arguments to the semaphore thread. 
 *       Right now we use the stack of the thread which calls up/down, 
 *       this might cause problems if the calling thread is manipulated
 *       (l4_thread_exregs, e.g during shutdown)
 * \todo How to recover from IPC errors?
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

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/env/errno.h>
#include <l4/util/macros.h>
#include <l4/thread/thread.h>

/* Lib includes */
#include <l4/semaphore/semaphore.h>
#include "__config.h"
#include "__debug.h"

/*****************************************************************************
 *** types / defines 
 *****************************************************************************/

/**
 * \brief Semaphore wait queue 
 */
typedef struct l4sem_wq
{
  l4_threadid_t     thread;  ///< thread waiting on the semaphore
  l4_prio_t         prio;    ///< thread prio
  struct l4sem_wq * next;    ///< next element in wait queue
} l4sem_wq_t;

/*****************************************************************************
 *** global data 
 *****************************************************************************/

/* Semaphore thread id */
l4_threadid_t l4semaphore_thread_l4_id  = L4_INVALID_ID;
static l4thread_t l4semaphore_thread_id = L4THREAD_INVALID_ID;

#ifdef L4API_l4x0
l4_uint32_t l4semaphore_thread_l4_id32 = -1;
#endif

/* wait queue allocation */
static l4sem_wq_t wq_entries[L4SEMAPHORE_MAX_WQ_ENTRIES];
static int next_entry = 0;

/*****************************************************************************
 *** performance measurements
 *****************************************************************************/

/*****************************************************************************
 *** wait queue entry allocation
 ***
 *** We know the max. number of wait queue entries we have to provide
 *** (each thread can only block on one sempahore at a time, and we have a 
 *** limited number of threads per task), so its save to use a static array
 *** for the allocation. If we do not know the max. number of threads (as 
 *** with upcoming L4 APIs), we must replace the allocation e.g. with 
 *** the L4Env slab allocator, but this will have an impact of the 
 *** performance, especially with releasing a wait queue entry.
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Allocate wait queue entry
 * 
 * \return Pointer to wait queue entry, NULL if no entry available
 */
/*****************************************************************************/ 
static inline l4sem_wq_t *
__alloc_entry(void)
{
  int i = next_entry;
  int found = 0;

  /* find unused wait queue entry */
  do
    {
      if (l4_is_invalid_id(wq_entries[i].thread))
        found = 1;
      else
        i = (i + 1) % L4SEMAPHORE_MAX_WQ_ENTRIES;	  
    }
  while (!found && (i != next_entry));

  if (!found)
    {
      Panic("L4semaphore: no wait queue entry available!");
      return NULL;
    }
  next_entry = (i + 1) % L4SEMAPHORE_MAX_WQ_ENTRIES;

  return &wq_entries[i];
}

/*****************************************************************************/
/**
 * \brief  Free wait queue entry
 * 
 * \param  wq            Wait queue entry
 */
/*****************************************************************************/ 
static inline void
__free_entry(l4sem_wq_t * wq)
{
  /* free */
  wq->thread = L4_INVALID_ID;
}

/*****************************************************************************
 *** helpers
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Enqueue thread into semaphore wait queue
 * 
 * \param  sem           Semaphore
 * \param  t             Thread id
 */
/*****************************************************************************/ 
static inline void
__enqueue_thread(l4semaphore_t * sem, l4_threadid_t t)
{
#if L4SEMAPHORE_SORT_WQ
  l4sem_wq_t * wq, * wp;
  l4_prio_t prio;

  /* get thread priority */
  prio = l4thread_get_prio(l4thread_id(t));
  if (prio < 0)
    {
      Error("L4Semaphore: failed to get priority for thread "IdFmt,IdStr(t));
      prio = 0;
    }

  /* insert thread into wait queue */
  wq = __alloc_entry();
  wq->thread = t;
  wq->prio = prio;
  wq->next = NULL;
  if (sem->queue == NULL)
    sem->queue = wq;
  else
    {
      if (((l4sem_wq_t *)(sem->queue))->prio < prio)
        {
          wq->next = sem->queue;
          sem->queue = wq;
        }
      else
        {
          wp = sem->queue;
          while ((wp->next != NULL) && (wp->next->prio >= prio))
            wp = wp->next;

          wq->next = wp->next;
          wp->next = wq;
        }
    }

#else /* !L4SEMAPHORE_SORT_WQ */
  l4sem_wq_t * wq, * wp;

  /* insert thread into wait queue */
  wq = __alloc_entry();
  wq->thread = t;
  wq->prio = 0;
  wq->next = NULL;
  if (sem->queue == NULL)
    sem->queue = wq;
  else
    {
      wp = sem->queue;
      while (wp->next)
        wp = wp->next;
      wp->next = wq;
    }
#endif
}

/*****************************************************************************/
/**
 * \brief  Wakeup thread.
 * 
 * \param  t             L4 thread id.
 *
 * Send reply to blocked thread.
 */
/*****************************************************************************/ 
static inline void
__wakeup_thread(l4_threadid_t t)
{
  int error;
  l4_msgdope_t result;

#if L4SEMAPHORE_SEND_ONLY_IPC
  error = l4_i386_ipc_send(t,(void *)(L4_IPC_SHORT_MSG | L4_IPC_DECEIT_MASK),
                           0,0,L4_IPC_TIMEOUT(0,1,0,0,0,0),&result);
#else
  error = l4_i386_ipc_send(t,L4_IPC_SHORT_MSG,0,0,L4_IPC_TIMEOUT(0,1,0,0,0,0),
                           &result);
#endif

  if ((error) && (error != L4_IPC_SETIMEOUT))
    Error("L4semaphore: wakeup message failed (0x%02x)!",error);

}

/*****************************************************************************/
/**
 * \brief Semaphore thread.
 *
 * This thread is used by other threads to block if they did not get the 
 * semaphore. Threads call the semaphore thread , it enqueues the caller to 
 * the semaphores wait queue (using the wait queue element provided in the 
 * message buffer by the caller thread). The thread owning the semaphore 
 * calls the semaphore thread to wakeup other threads waiting on that semaphore.
 * The semaphore thread then sends the reply message the the waiting thread.
 *
 * A separate thread is used to serialize the accesses to the wait queues of
 * the semaphores, thus no additional synchronization is necessary.  
 *
 * There is one tricky situation if the wakeup message for a semaphore is 
 * received before the block message. This might happen if the thread waiting
 * on that semaphore is preemted by the thread releasing the semaphore before 
 * it can send the message to the semaphore thread (see l4semaphore_down()).
 * In this case the semaphore thread receives a wakeup message but no thread 
 * is enqueued in the wait queue. In such situations, the wakeup message is 
 * saved in l4semaphore::pending and the next block message is replied 
 * immediately.
 */
/*****************************************************************************/ 
static void 
l4semaphore_thread(void * data)
{
  int error,i;
  l4_uint32_t dw0,dw1;
  l4_threadid_t me,src,wakeup;
  l4semaphore_t * sem;
  l4sem_wq_t * wp;
  l4_msgdope_t result;

  /* get my L4 id, required to check the sender of a message */
  me = l4thread_l4_id(l4thread_myself());

#if DEBUG_INIT
  INFO("semaphore thread: "IdFmt"\n",IdStr(me));
#endif

  /* setup wait queue entry allocation */
  for (i = 0; i < L4SEMAPHORE_MAX_WQ_ENTRIES; i++)
    wq_entries[i].thread = L4_INVALID_ID;

  /* semaphore thread loop */
  while (1)
    {
      /* wait for request */
      error = l4_i386_ipc_wait(&src,L4_IPC_SHORT_MSG,&dw0,&dw1,L4_IPC_NEVER,
                               &result);
      if (!error)
        {
          if (!l4_task_equal(me,src))
            {
              Error("L4semaphore: ignored request from other task "
                    "("IdFmt", I'm "IdFmt")!",IdStr(src),IdStr(me));
              continue;
            }
	  
          /* dw1 -> pointer to message struct */
          sem = (l4semaphore_t *)dw1;
          if (sem == NULL)
            {
              Error("L4semaphore: invalid request!");
              continue;
            }
	  
          switch(dw0)
            {
            case L4SEMAPHORE_BLOCK:
              /* block thread, enqueue to semaphores wait queue */
              if (sem->pending > 0)
                {
                  /* we already got the wakeup message, return immediately */
                  sem->pending--;
                  __wakeup_thread(src);
                }
              else
                __enqueue_thread(sem,src);

              /* done */
              break;
	      
            case L4SEMAPHORE_RELEASE:
              /* wakeup thread */
              if (sem->queue == NULL)
                {
                  /* no thread in waitqueue, store wakeup */
                  sem->pending++;
                  wakeup = L4_INVALID_ID;
                }
              else
                {
                  /* remove thread from wait queue */
                  wp = sem->queue;
                  sem->queue = wp->next;
                  wakeup = wp->thread;
                  __free_entry(wp);
                }

#if !(L4SEMAPHORE_SEND_ONLY_IPC)
              /* reply */
              l4_i386_ipc_send(src,L4_IPC_SHORT_MSG,0,0,
                               L4_IPC_TIMEOUT(0,1,0,0,0,0),&result);
#endif

              if (!l4_is_invalid_id(wakeup))
                __wakeup_thread(wakeup);

              /* done */
              break;
	      
            default:
              Error("L4semaphore: invalid request!");		  
            }
        }
      else
        Error("L4semaphore: IPC error 0x%02x!",error);
    }
  
  /* this should never happen... */
  Panic("L4semaphore: left semaphore thread!?");
}

/*****************************************************************************
 *** API functions
 *****************************************************************************/

/*****************************************************************************/
/**
 * \brief  Library initialization
 * \ingroup init
 *
 * \return 0 on success, -1 if setup failed.
 * 
 * Setup semaphore thread.
 */
/*****************************************************************************/
int
l4semaphore_init(void)
{
  l4thread_t t;

  /* start semaphore thread */
  t = l4thread_create_long(L4THREAD_INVALID_ID,l4semaphore_thread,
                           L4THREAD_INVALID_SP,L4SEMAPHORE_THREAD_STACK_SIZE,
                           L4SEMAPHORE_THREAD_PRIO,NULL,
                           L4THREAD_CREATE_ASYNC | L4THREAD_CREATE_SETUP);

  if (t < 0)
    {
      Error("L4semaphore: failed to create semaphore thread: %s (%d)!",
            l4env_errstr(t),t);
      return -1;
    }

  /* set semaphore thread id */
  l4semaphore_thread_id = t;
  l4semaphore_thread_l4_id = l4thread_l4_id(t);

#if DEBUG_INIT
  INFO("started semaphore thread (%d,"IdFmt")\n",t,
       IdStr(l4semaphore_thread_l4_id));
#endif

#ifdef L4API_l4x0
  l4semaphore_thread_l4_id32 = l4sys_to_id32(l4semaphore_thread_l4_id);
#endif

  /* done */
  return 0;
}

/*****************************************************************************/
/**
 * \brief  Set semaphore thread priority.
 * \ingroup init
 *
 * \param  prio          Priority
 *	
 * \return 0 on success, error code if failed.
 */
/*****************************************************************************/ 
int
l4semaphore_set_thread_prio(l4_prio_t prio)
{
  /* set priority */
  if (l4semaphore_thread_id != L4THREAD_INVALID_ID)
    return l4thread_set_prio(l4semaphore_thread_id,prio);
  else
    return -L4_EINVAL;
}
  
