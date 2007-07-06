/*****************************************************************************/
/**
 * \file   semaphore/include/test.h
 * \brief  Delayed preemption based semaphore implementation without
 *         serializer thread
 *
 * \date   04/05/2007
 * \author Alexander Boettcher <boettcher@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2007 Technische Universitaet Dresden
 * This file is part of TUDOS. Please see the COPYING file for license details.
 */

#ifndef SEM_WO_SER_THREAD
#define SEM_WO_SER_THREAD 0

/* L4 includes */
#include <l4/sys/ipc.h>
#include <l4/util/macros.h> //Assert
#include <l4/util/util.h>   //l4_sleep_forever
#include <l4/sigma0/kip.h>
#include <l4/thread/thread.h>
#include <l4/log/l4log.h>

#define SET_DELAY_PREEMPTION Assert(!l4_utcb_dp_overrun()); l4_utcb_dp_set()
#define UNSET_DP_AND_CHECK_ODP Assert(!l4_utcb_dp_overrun()); l4_utcb_dp_unset(); if (l4_utcb_dp_triggered()) l4_yield(); Assert(!l4_utcb_dp_overrun());

struct dp_sem_wq {
 l4thread_t thread;
 int prio;
 struct dp_sem_wq volatile * next; 
 struct dp_sem_wq volatile * prev; 
};

typedef struct dp_sem {
 volatile long counter;
 volatile unsigned long version;
 struct dp_sem_wq volatile * waitq;
} dp_sem_t;

L4_INLINE void dp_sem_init(dp_sem_t * sem);
L4_INLINE void dp_sem_up(dp_sem_t * sem);
L4_INLINE void dp_sem_down(dp_sem_t * sem);
L4_INLINE int dp_sem_try_down(dp_sem_t * sem);
L4_INLINE int dp_sem_down_timed(dp_sem_t * sem, unsigned time);

L4_INLINE void
dp_sem_init(dp_sem_t * sem)
{
  Assert(l4sigma0_kip_kernel_has_feature("utcb"));
  sem->counter = 1;
  sem->waitq   = NULL;
  sem->version = 0;
}

L4_INLINE void dp_sem_up(dp_sem_t * sem)
{
  SET_DELAY_PREEMPTION;
  /* increment semaphore counter */
  sem->counter ++;

  if (sem->counter > 0) {
    UNSET_DP_AND_CHECK_ODP;
  }
  else
  {
    l4_msgdope_t result;

    /* sanity checks */
    Assert(sem->counter <= 0);
    Assert(sem->waitq != NULL);
    Assert(sem->waitq->prev == NULL);

    /* wake the first one in queue */
    struct dp_sem_wq * entry = sem->waitq;
    sem->waitq = sem->waitq->next;
    if (sem->waitq != NULL)
      sem->waitq->prev = NULL;
    else
      Assert(sem->counter == 0);
    
    sem->version ++;

    //unset DP done implicitly by IPC
    l4_ipc_send(l4thread_l4_id(entry->thread), (void *)(L4_IPC_SHORT_MSG | L4_IPC_DECEIT_MASK),
                0, 0, L4_IPC_NEVER, &result);
    Assert(!l4_utcb_dp_triggered());
  }
}

int dp_sem_down_general(dp_sem_t * sem, struct dp_sem_wq *entry);
int dp_sem_down_general(dp_sem_t * sem, struct dp_sem_wq *entry) {
  l4thread_t myself;
  int prio;
  int last = 0;  
  unsigned long version;
  struct dp_sem_wq * next;

  UNSET_DP_AND_CHECK_ODP;
  /* iterate through list */
  myself = l4thread_myself();
  prio   = l4thread_get_prio(myself);

  Assert(!l4_utcb_dp_triggered());
  SET_DELAY_PREEMPTION;

  next = sem->waitq;
  version = sem->version;

  while (next != NULL && prio <= next->prio) {
    if (next->next == NULL) {
      last = 1;
      break;
    }
    next = next->next;
    if (l4_utcb_dp_triggered()) {
      l4_yield();
      SET_DELAY_PREEMPTION;
      Assert(!l4_utcb_dp_triggered());

      if (sem->version != version) {
        if (sem->counter > 0 ) {
          /* decrement counter, check result */
          sem->counter --;
  
          UNSET_DP_AND_CHECK_ODP;
           
          return 1;
        } else {
          /* only low priority threads don't find a position early enough */
          /* restart enqueue process, because waitqueue changed */
          version = sem->version;
          next = sem->waitq; 
        }
      }
    }
  }

  /* decrement counter, check result */
  sem->counter --;
  sem->version ++;

  entry->thread = myself;
  entry->prio   = prio;

  if (last) {
    Assert(last == 1);
    Assert(sem->counter < 0);
    Assert(next->next == NULL);

    entry->next   = NULL;
    entry->prev   = next;

    next->next = entry; 
  } else {
    if (next == NULL) {
      Assert(sem->waitq == NULL);

      if (sem->counter >= 0 ) {
          UNSET_DP_AND_CHECK_ODP;

          return 1;
      }

      Assert(sem->counter == -1);

      entry->next   = NULL;
      entry->prev   = NULL;

      sem->waitq = entry;
    } else {

      entry->next   = next;
      entry->prev   = next->prev;

      next->prev->next = entry;
      next->prev = entry;
    }
  }

  return 0;

}

L4_INLINE void dp_sem_down(dp_sem_t * sem)
{
  SET_DELAY_PREEMPTION;
  if (sem->counter > 0 )
  {
    /* decrement counter, check result */
    sem->counter --;
  
    UNSET_DP_AND_CHECK_ODP;
  }
  else
  {
    l4_threadid_t src;
    int error;
    l4_umword_t dw0, dw1;
    l4_msgdope_t result;
    struct dp_sem_wq entry;

    if (dp_sem_down_general(sem, &entry)) {
      Assert(!l4_utcb_dp_triggered());
      return;
    }

    /* wait for wake up IPC*/
    error = l4_ipc_wait(&src, L4_IPC_SHORT_MSG, &dw0, &dw1, L4_IPC_NEVER,
                        &result);
    // use sleep with ex-regs instead of open wait ?
    // l4_sleep_forever();
    Assert(!l4_utcb_dp_triggered());

  }
}

/*****************************************************************************
 * decrement semaphore counter, return error if semaphore locked
 *****************************************************************************/
L4_INLINE int
dp_sem_try_down(dp_sem_t * sem)
{
  if (sem->counter <= 0)
    return 0;

  SET_DELAY_PREEMPTION;

  if (sem->counter <= 0) {
    UNSET_DP_AND_CHECK_ODP;
    return 0;
  }
  
  sem->counter --;  
  ASSERT(sem->counter >= 0);
  UNSET_DP_AND_CHECK_ODP;
  return 1;
}

/*****************************************************************************
 * decrement semaphore counter, block for a given time if semaphore locked
 *****************************************************************************/
L4_INLINE int
dp_sem_down_timed(dp_sem_t * sem, unsigned timeout){
  SET_DELAY_PREEMPTION;
  if (sem->counter > 0 )
  {
    /* decrement counter, check result */
    sem->counter --;
  
    UNSET_DP_AND_CHECK_ODP;
    return 1;
  }

  struct dp_sem_wq entry;
  //take timestamp
  if (dp_sem_down_general(sem, &entry))
    return 1;
  //take timestamp diff
  //TODO adjust time, waittime - enqueue time  !
  //TODO TIMEOUTS
  //TODO exregs     
  //l4_sleep(time); 
  //int e, m;
      
  l4_threadid_t src;
  int error;
  l4_umword_t dw0, dw1;
  l4_msgdope_t result;

//  l4util_micros2l4to((signed)timeout*1000, &m, &e);
//  error = l4_ipc_wait(&src, L4_IPC_SHORT_MSG, &dw0, &dw1, L4_IPC_TIMEOUT(0, 0, m, e, 0, 0),
//                        &result);
  error = l4_ipc_wait(&src, L4_IPC_SHORT_MSG, &dw0, &dw1,
  			l4_timeout(L4_IPC_TIMEOUT_NEVER, l4util_micros2l4to((signed)timeout*1000)),
                        &result);
  //TODO check whether right src send the wakeup
  if (error != 0) {
    SET_DELAY_PREEMPTION;
    
    /* decrement counter, check result */
    sem->counter ++;
  
    UNSET_DP_AND_CHECK_ODP;
    return 0;
  }

  return 1;
}
#endif
