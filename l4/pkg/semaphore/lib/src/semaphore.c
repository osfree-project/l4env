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
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/sigma0/kip.h>
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
  struct l4sem_wq * prev;    ///< prev element in wait queue
} l4sem_wq_t;

/*****************************************************************************
 *** global data 
 *****************************************************************************/

/* Semaphore thread id */
l4_threadid_t l4semaphore_thread_l4_id  = L4_INVALID_ID;
static l4thread_t l4semaphore_thread_id = L4THREAD_INVALID_ID;

/* wait queue allocation */
static l4sem_wq_t wq_entries[L4SEMAPHORE_MAX_WQ_ENTRIES];
static int next_entry = 0;

#define WAKEUP_NOWAIT 1 /* wakeup a thread - do not wait when it is not ready */
#define WAKEUP_WAIT 0 /* wakeup a thread - wait until it is ready */

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

  if (wq->prev != NULL)
    wq->prev->next = wq->next;
  if (wq->next != NULL)
    wq->next->prev = wq->prev;
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
static inline l4sem_wq_t *
__enqueue_thread(l4semaphore_t * sem, l4_threadid_t t)
{
#if L4SEMAPHORE_SORT_WQ
  l4sem_wq_t * wq, * wp;
  l4_prio_t prio;

  /* get thread priority */
  prio = l4thread_get_prio(l4thread_id(t));
  if (prio < 0)
    {
      /* Could not get prio of thread. This could either mean that the thread 
       * got killed in the meantime or that the semaphore lib was used by a 
       * thread which is not managed by the thread library. In both cases the 
       * thread library has no valid tcb for the thread.
       * Use prio 0, this enqueues thread at the end of the wait queue.
       */
      LOG_Error("l4semaphore: failed to get priority of thread "l4util_idfmt \
                ": %s (%d)", l4util_idstr(t), l4env_errstr(prio), prio);
      prio = 0;
    }

  /* insert thread into wait queue */
  wq = __alloc_entry();
  wq->thread = t;
  wq->prio = prio;
  wq->next = NULL;
  if (sem->queue == NULL)
    {
      wq->prev   = NULL;
      sem->queue = wq;
    }
  else
    {
      if (((l4sem_wq_t *)(sem->queue))->prio < prio)
        {
          wq->next = sem->queue;
          wq->prev = NULL;
          ((l4sem_wq_t *)sem->queue)->prev = wq;
          sem->queue = wq;
        }
      else
        {
          wp = sem->queue;
          while ((wp->next != NULL) && (wp->next->prio >= prio))
            wp = wp->next;

          wq->prev = wp;
          wq->next = wp->next;
          if (wp->next != NULL)
            wp->next->prev = wq;
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
    {
      wq->prev   = NULL;
      sem->queue = wq;
    }
  else
    {
      wp = sem->queue;
      while (wp->next)
        wp = wp->next;
      wp->next = wq;
      wq->prev = wp;
    }
#endif
  return wq;
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
static inline int
__wakeup_thread(l4_threadid_t t, int nowait)
{
  int error;
  l4_msgdope_t result;
  l4_timeout_t to;

#if L4SEMAPHORE_RESTART_IPC
  /* zero send timeout */
  if (nowait)
    to = L4_IPC_SEND_TIMEOUT_0;
  /* wait as long as necessary*/
  else
    to = L4_IPC_NEVER;
#else
  /* zero send timeout */
  to = L4_IPC_SEND_TIMEOUT_0;
#endif

#if L4SEMAPHORE_SEND_ONLY_IPC
  error = l4_ipc_send(t, (void *)(L4_IPC_SHORT_MSG | L4_IPC_DECEIT_MASK),
                      0, 0, to, &result);
#else
  error = l4_ipc_send(t, (void *)(L4_IPC_SHORT_MSG | L4_IPC_DECEIT_MASK), 0, 0, to, &result);
#endif

  if (error)
    {
      if (error != L4_IPC_SETIMEOUT)
        LOG_Error("L4semaphore: wakeup message failed (0x%02x)!", error);
      return 0;
    }

  return 1;
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
  int error,i,datakey,found;
  l4_umword_t dw0,dw1;
  l4_threadid_t me,src,wakeup;
  l4semaphore_t * sem;
  l4sem_wq_t * wp;
  l4_msgdope_t result;
  void * vwp;

  /* get my L4 id, required to check the sender of a message */
  me = l4thread_l4_id(l4thread_myself());

  datakey = l4thread_data_allocate_key();
  if (datakey==-L4_ENOKEY)
    {
      LOG_Error("L4semaphore: can not allocate data key!");
      /* this should never happen... */
      Panic("L4semaphore: left semaphore thread!?");
      return;
    }

  LOGdL(DEBUG_INIT, "semaphore thread: "l4util_idfmt, l4util_idstr(me));

  /* setup wait queue entry allocation */
  for (i = 0; i < L4SEMAPHORE_MAX_WQ_ENTRIES; i++)
    wq_entries[i].thread = L4_INVALID_ID;

  /* semaphore thread loop */
  while (1)
    {
      /* wait for request */
      error = l4_ipc_wait(&src, L4_IPC_SHORT_MSG, &dw0, &dw1, L4_IPC_NEVER,
                          &result);
      if (!error)
        {
          if (!l4_task_equal(me, src))
            {
              LOG_Error("L4semaphore: ignored request from other task "
                        "("l4util_idfmt", I'm "l4util_idfmt")!", 
                        l4util_idstr(src), l4util_idstr(me));
              continue;
            }
	  
          /* dw1 -> pointer to message struct */
          sem = (l4semaphore_t *)dw1;
          if (sem == NULL)
            {
              LOG_Error("L4semaphore: invalid request!");
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
                  __wakeup_thread(src,WAKEUP_WAIT);
                }
              else 
                __enqueue_thread(sem, src);

              /* done */
              break;
            case L4SEMAPHORE_BLOCKTIMED:
              if (sem->pending > 0)
	        {
                  /* we already got the wakeup message, return immediately */
                  if (__wakeup_thread(src,WAKEUP_NOWAIT))
                    /* the IPC was received by the waiting thread*/
                    sem->pending--;
                    /*else - thread did not wait, because of a time out*/
		}  
	      else
	        {
                  wp = __enqueue_thread(sem, src);
                  /* mark thread, set the pointer of the entry in the queue 
		   * to find it later easily */
                  l4thread_data_set(l4thread_id(src),datakey,wp);
                }
              break;
            case L4SEMAPHORE_RELEASE:

#if !(L4SEMAPHORE_SEND_ONLY_IPC)
              /* reply */
#if L4SEMAPHORE_RESTART_IPC
              l4_ipc_send(src, (void *)(L4_IPC_SHORT_MSG | L4_IPC_DECEIT_MASK),
			  0, 0, L4_IPC_NEVER, &result);
#else
              l4_ipc_send(src, (void *)(L4_IPC_SHORT_MSG | L4_IPC_DECEIT_MASK),
			  0, 0, L4_IPC_SEND_TIMEOUT_0, &result);
#endif
#endif
              found = 0;
              /* look for a waiting thread*/
              while (sem->queue != NULL)
	        {
                  wp               = sem->queue;
                  sem->queue       = wp->next;
                  wakeup           = wp->thread;
                  /* remove thread from wait queue */
                  __free_entry(wp);

                  /* thread exists ?*/
                  if (!l4_is_invalid_id(wakeup))
		    {
                      vwp = l4thread_data_get(l4thread_id(wakeup),datakey);
                      if (vwp != NULL)
		        {
                          /* thread, which called semaphore_down_timed*/
                          /* unmark thread*/
                          l4thread_data_set(l4thread_id(wakeup),datakey,NULL);
                          /* try to notify the thread*/
                          if (__wakeup_thread(wakeup,WAKEUP_NOWAIT))
			    {
                              found = 1; /* success, thread wait */
                              break;
                            } 
			  /* else: thread did not wait - forget this thread */
                        }
                      else
		        {
                          /* thread, which called semaphore_down */
                          found = 1; /* found a thread*/
                          __wakeup_thread(wakeup,WAKEUP_WAIT);
                          break;
                        }
		    }
		  else
                    LOG_Error("L4semaphore: invalid thread "l4util_idfmt"!"
			      "Maybe not alive ?", l4util_idstr(wakeup));
                }

              if (!found)
                sem->pending ++; /* store wakeup  */
              /* done */
              break;
            case L4SEMAPHORE_RELEASETIMED:
              vwp = l4thread_data_get(l4thread_id(src),datakey);
              /* vwp==NULL - another thread, which called L4SEMAPHORE_RELEASE, 
                             already dequeued the thread 'src' */
              if (vwp != NULL)
	        {
                  wp  = (l4sem_wq_t *)vwp;

                  if (wp->prev == NULL)
		    /* the first thread in the queue */
		    sem->queue = wp->next;
		  
                  /* remove thread from wait queue*/
                  __free_entry(wp);
                  /* unmark thread */
                  l4thread_data_set(l4thread_id(src),datakey,NULL);
                }
              /* done */
              break;
            default:
              LOG_Error("L4semaphore: invalid request!");		  
            }
        }
      else
        {
          if (error != L4_IPC_SETIMEOUT) 
            LOG_Error("L4semaphore: IPC error 0x%02x!", error);
        }
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

  if (!l4sigma0_kip_kernel_has_feature("deceit_bit_disables_switch"))
    {
      LOG_Error("Missing 'deceit_bit_disables_switch' kernel feature!");
      return -1;
    }



  /* start semaphore thread */
  t = l4thread_create_long(L4THREAD_INVALID_ID, l4semaphore_thread,
  			   ".sem", L4THREAD_INVALID_SP, 
                           L4SEMAPHORE_THREAD_STACK_SIZE,
                           L4SEMAPHORE_THREAD_PRIO, NULL,
                           L4THREAD_CREATE_ASYNC | L4THREAD_CREATE_SETUP);

  if (t < 0)
    {
      LOG_Error("L4semaphore: failed to create semaphore thread: %s (%d)!",
                l4env_errstr(t), t);
      return -1;
    }

  /* set semaphore thread id */
  l4semaphore_thread_id = t;
  l4semaphore_thread_l4_id = l4thread_l4_id(t);

  LOGdL(DEBUG_INIT, "started semaphore thread (%d,"l4util_idfmt")", 
        t, l4util_idstr(l4semaphore_thread_l4_id));

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

/*****************************************************************************/
/**
 * \brief  Restart semaphore down operation after previous IPC was canceled.
 * \param  sem	the semaphore
 *
 * \return 0 on success, error code if failed.
 */
/*****************************************************************************/
#if L4SEMAPHORE_RESTART_IPC
void
l4semaphore_restart_up(l4semaphore_t *sem, l4_msgdope_t result)
{
  l4_umword_t dummy;

  while (L4_IPC_ERROR(result) == L4_IPC_SECANCELED)
    {
      l4_ipc_call(l4semaphore_thread_l4_id, L4_IPC_SHORT_MSG,
                  L4SEMAPHORE_RELEASE, (l4_umword_t)sem, L4_IPC_SHORT_MSG,
                  &dummy, &dummy, L4_IPC_NEVER, &result); 
    }

  while (L4_IPC_ERROR(result) == L4_IPC_RECANCELED)
    {
      l4_ipc_receive(l4semaphore_thread_l4_id, L4_IPC_SHORT_MSG,
                     &dummy, &dummy, L4_IPC_NEVER, &result); 
    }
}
#endif

/*****************************************************************************/
/**
 * \brief  Restart semaphore up operation after previous IPC was canceled.
 * \param  sem	the semaphore
 *
 * \return 0 on success, error code if failed.
 */
/*****************************************************************************/
#if L4SEMAPHORE_RESTART_IPC
void
l4semaphore_restart_down(l4semaphore_t *sem,l4_msgdope_t result)
{
  l4_umword_t dummy;

  while (L4_IPC_ERROR(result) == L4_IPC_SECANCELED)
    {
      l4_ipc_call(l4semaphore_thread_l4_id, L4_IPC_SHORT_MSG, 
                  L4SEMAPHORE_BLOCK, (l4_umword_t)sem, L4_IPC_SHORT_MSG, 
                  &dummy, &dummy, L4_IPC_NEVER, &result); 
    }

#if !(L4SEMAPHORE_SEND_ONLY_IPC)
  while (L4_IPC_ERROR(result) == L4_IPC_RECANCELED)
    {
      l4_ipc_receive(l4semaphore_thread_l4_id, L4_IPC_SHORT_MSG,
                     &dummy, &dummy, L4_IPC_NEVER, &result); 
    }
#endif
}
#endif
