/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux/lib/src/time.c
 * \brief  Time and Timers
 *
 * \date   08/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/** \ingroup mod_common
 * \defgroup mod_time Time and Timer Implementation
 *
 * This module emulates one-shot kernel timers inside the Linux kernel.
 *
 * One-shot timers in Linux are executed as a bottom half (TIMER_BH). Therefore
 * only other BHs are prohibited while a timer function is executed.
 *
 * Requirements: (additionally to \ref pg_req)
 *
 * - \c jiffies and \c HZ have to be implemented as time base (perhaps using
 * l4io)
 */

/* L4 */
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/thread/thread.h>
#include <l4/lock/lock.h>
#include <l4/util/util.h>
#include <l4/env/errno.h>

#include <l4/dde_linux/dde.h>

/* Linux */
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <asm/softirq.h>

/* local */
#include "internal.h"
#include "__config.h"

/** \name Linux Timer Handling
 *
 * <em>This is from kernel/timer.c</em>
 * @{ */

/** timer list */
static struct list_head timer_list = LIST_HEAD_INIT(timer_list);

/** timer list access lock */
static l4lock_t timerlist_lock = L4LOCK_UNLOCKED;

/** timer thread id */
static l4thread_t timer_tid = L4THREAD_INVALID_ID;

/** initialization flag */
static int _initialized = 0;

/** Restart Timer Thread */
static inline void __restart_timer_thread(void)
{
  l4_threadid_t timer_l4id;
  l4_msgdope_t result;
  int err;

  timer_l4id = l4thread_l4_id(timer_tid);

#if 0
  /* do not notify myself about list update (it's checked later in timer
    loop) */
  if (l4_thread_equal(timer_l4id, l4_myself()))
    return;
#endif

  /* send notification with timeout 0 so don't block here while holding
     timerlist_lock */
  do
    {
      err = l4_ipc_send(timer_l4id,
                        L4_IPC_SHORT_MSG, 0, 0,
                        L4_IPC_SEND_TIMEOUT_0, &result);

      if (err == L4_IPC_SETIMEOUT)
        break;

      if (err)
        LOGdL(DEBUG_ERRORS, "Error: IPC error 0x%02lx in __restart()", 
              L4_IPC_ERROR(result));
    }
  while (err);
}

/** Adding One-Shot Timer To List Helper */
static inline void __internal_add_timer(struct timer_list *timer)
{
  /*
   * must be cli-ed when calling this
   */
  struct list_head *tmp;

  list_for_each(tmp, &timer_list)
  {
    struct timer_list *tp = list_entry(tmp, struct timer_list, list);

    if (tp->expires > timer->expires)
      break;
  }

  list_add_tail(&timer->list, tmp);

#if DEBUG_TIMER
  LOG("ADDED timer (current list follows)");
  list_for_each(tmp, &timer_list)
  {
    struct timer_list *tp = list_entry(tmp, struct timer_list, list);
    LOG("  [%p] expires %lu, data = %lu", tmp, tp->expires, tp->data);
  }
#endif

  /* do we need to restart the timer thread? */
  if (timer->list.prev == &timer_list)
    {
      LOGd(DEBUG_TIMER, "  ... RESTART timer thread");
      __restart_timer_thread();
    }

  LOGd(DEBUG_TIMER, " ");
}

/** Removing One-Shot Timer From List Helper */
static inline int __internal_detach_timer(struct timer_list *timer)
{
  /*
   * must be cli-ed when calling this
   */
#if DEBUG_TIMER
  struct list_head *tmp;
#endif

  if (!timer_pending(timer))
    {
#if DEBUG_TIMER
//      LOG("Oops, attempt to detach not pending timer.\n");
#endif
      return 0;
    }
  list_del(&timer->list);

#if DEBUG_TIMER
  LOG("DETACHED timer (current list follows)");
  list_for_each(tmp, &timer_list)
  {
    struct timer_list *tp = list_entry(tmp, struct timer_list, list);
    LOG("  [%p] expires %lu, data = %lu",
           tmp, tp->expires, tp->data);
  }
  LOG("");
#endif
  return 1;
}

/** Add One-Shot Timer
 * \ingroup mod_time
 *
 * \param timer  timer reference to be added
 */
void add_timer(struct timer_list *timer)
{
  l4lock_lock(&timerlist_lock);
  if (timer_pending(timer))
    goto bug;
  __internal_add_timer(timer);
  l4lock_unlock(&timerlist_lock);
  return;
bug:
  l4lock_unlock(&timerlist_lock);
  LOG_Error("kernel timer added twice.");
}

/** Modify One-Shot Timer
 * \ingroup mod_time
 *
 * \param timer    timer reference to be modified
 * \param expires  new expiration time
 *
 * \return 0 if timer is pending; 1 if modification was okay
 */
int mod_timer(struct timer_list *timer, unsigned long expires)
{
  int ret;

  l4lock_lock(&timerlist_lock);
  timer->expires = expires;
  ret = __internal_detach_timer(timer);
  __internal_add_timer(timer);
  l4lock_unlock(&timerlist_lock);
  return ret;
}

/** Delete One-Shot Timer
 * \ingroup mod_time
 *
 * \param timer  timer reference to be removed
 *
 * \return 0 if timer is pending; 1 if deletion completed
 */
int del_timer(struct timer_list *timer)
{
  int ret;

  l4lock_lock(&timerlist_lock);
  ret = __internal_detach_timer(timer);
  timer->list.next = timer->list.prev = NULL;
  l4lock_unlock(&timerlist_lock);
  return ret;
}

/** Delete One-Shot Timer (SMP sync)
 * \ingroup mod_time
 *
 * \param timer  timer reference to be removed
 *
 * \return 0 if timer is pending; 1 if deletion completed
 *
 * \note In non-SMP Mode, del_timer_sync is a define to del_timer()
 */
#ifdef CONFIG_SMP
int del_timer_sync(struct timer_list *timer)
{
  int ret;

  l4lock_lock(&timerlist_lock);
  ret = __internal_detach_timer(timer);
  timer->list.next = timer->list.prev = NULL;
  l4lock_unlock(&timerlist_lock);
  return ret;
}
#endif /* !CONFIG_SMP */

/** Timer Synchronization Dummy
 * \ingroup mod_time
 *
 * However, in non-SMP mode, it is an empty define
 */
#ifdef CONFIG_SMP
void sync_timers(void)
{
#warning sync_timers() is not implemented
  LOG_Error("Not implemented");
}
#endif /* !CONFIG_SMP */

/** Timer Sleep Implementation
 *
 * \param  to  IPC timeout
 *
 * \return 1 on timed out IPC (0 = message received and indicates \c
 * __restart)
 *
 * Has to be called holding the timerlist lock.
 */
static inline int __timer_sleep(l4_timeout_t to)
{
  l4_threadid_t me;
  l4_threadid_t any;
  l4_umword_t dummy;
  l4_msgdope_t result;
  int err;

  me = l4thread_l4_id(timer_tid);

  /* release lock */
  l4lock_unlock(&timerlist_lock);

  /* sleep */
  do
    {
      err = l4_ipc_wait(&any,
                             L4_IPC_SHORT_MSG, &dummy, &dummy, to, &result);

      if (err == L4_IPC_RETIMEOUT)
        break;

      if (err)
        {
          LOG_Error("in IPC (0x%02lx)", L4_IPC_ERROR(result));
          any = L4_INVALID_ID;
        }
    }
  while (!l4_task_equal(me, any));

  /* reacquire lock */
  l4lock_lock(&timerlist_lock);

  /* done */
  return (err ? 1 : 0);
}

/** @} */
/** Linux DDE Timer Thread
 *
 * We sleep for the most time: until next timer or L4_IPC_NEVER. If timer
 * expires release lock and do timer function.
 *
 * \todo priorities
 */
static void dde_timer_thread(void)
{
  l4_timeout_t to;
  int err, to_e, to_m;

  struct list_head *head = &timer_list;
  struct list_head *curr;
  struct timer_list *tp;

  unsigned long curr_jiffies;
  long diff;
  int to_us;

  timer_tid = l4thread_myself();
  if ((err = l4dde_process_add_worker())!=0)
     Panic("l4dde_process_add_worker() failed: %s", l4env_strerror(-err));

  ++local_bh_count(smp_processor_id());

  /* we are up */
  if (l4thread_started(NULL))
    goto ret;

  LOGd(DEBUG_MSG, "dde_timer_thread "l4util_idfmt" running.",
       l4util_idstr(l4thread_l4_id(l4thread_myself())));

  /* acquire lock */
  l4lock_lock(&timerlist_lock);

  /* timer loop */
  while (1)
    {
      LOGd(DEBUG_TIMER, "begin of timer_loop");
      curr = head->next;
      curr_jiffies = jiffies;

      /* no timer set? */
      if (curr == head)
        {
          to = L4_IPC_NEVER;

          /* wait for __restart */
          __timer_sleep(to);
          continue;
        }

      tp = list_entry(curr, struct timer_list, list);
      diff = tp->expires - curr_jiffies;

      LOGd(DEBUG_TIMER, 
           "timer_loop: jiffies = %lu, tp->expires = %lu, diff = %ld",
           curr_jiffies, tp->expires, diff);
      
      /* wait for next timer? */
      if (diff > 0)
        {
          to_us = diff * (1000000 / HZ);
          if ((err = l4util_micros2l4to(to_us, &to_m, &to_e)))
            {
              Panic("error on timeout calculation (us = %d)", to_us);
              continue;
            }
          LOGd(DEBUG_TIMER, 
               "timer_loop: to_us = %d, to_e = %d, to_m = %d",
               to_us, to_e, to_m);

          to = L4_IPC_TIMEOUT(0, 0, to_m, to_e, 0, 0);

          /* wait for timeout or __restart */
          __timer_sleep(to);
          continue;
        }

      /* detach from timer list */
      __internal_detach_timer(tp);
      LOGd(DEBUG_TIMER, "timer_loop: run timer %p", tp);

      /* release lock */
      l4lock_unlock(&timerlist_lock);

      /* run timed function */
      tp->function(tp->data);

      /* reaquire lock */
      l4lock_lock(&timerlist_lock);

      LOGd(DEBUG_TIMER, "timer_loop: jiffies = %lu, next = %p (head = %p)",
           curr_jiffies, head->next, head);
    }

 ret:
  /* that should never happen */
  Panic("left timer loop!");
  l4thread_exit();
}

/** Initialize Timer Thread
 * \ingroup mod_time
 *
 * \return 0 on success; negative error code otherwise
 *
 * One timer thread is created on initialization.
 */
int l4dde_time_init()
{
  int err;

  /* create timer thread */
  err = l4thread_create_long(L4THREAD_INVALID_ID,
                             (l4thread_fn_t)dde_timer_thread,
                             ".timer",
                             L4THREAD_INVALID_SP,
                             L4THREAD_DEFAULT_SIZE,
                             L4THREAD_DEFAULT_PRIO,
                             (void *) 0,
                             L4THREAD_CREATE_SYNC);

  if (err < 0)
    return err;

  ++_initialized;
  return 0;
}
