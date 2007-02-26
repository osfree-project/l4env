/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux/lib/src/softirq.c
 * \brief  Deferred Activities (BHs, Tasklets, real softirqs)
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
 * \defgroup mod_softirq Deferred Activities
 *
 * This module emulates <em>deferred activities</em> at interrupt level inside
 * the Linux kernel.
 *
 * Deferred activities in Linux can be \e old-style bottom halves, \e new-style
 * tasklets and softirqs.
 *
 * Requirements: (additionally to \ref pg_req)
 *
 * - none
 *
 * Configuration:
 *
 * - setup #SOFTIRQ_THREADS to configure number of softirq handler threads
 * <em>(NOT YET IMPLEMENTED)</em>
 */

/* L4 */
#include <l4/env/errno.h>
#include <l4/sys/syscalls.h>
#include <l4/thread/thread.h>
#include <l4/semaphore/semaphore.h>
#include <l4/lock/lock.h>

#include <l4/dde_linux/dde.h>

/* Linux */
#include <linux/config.h>
#include <linux/mm.h>
#include <linux/kernel_stat.h>
#include <linux/interrupt.h>
#include <linux/smp_lock.h>
#include <linux/init.h>
#include <linux/tqueue.h>

/* local */
#include "internal.h"
#include "__config.h"

/** \name Module variables
 *
 * \krishna We want to support more than one softirq thread (\c
 * SOFTIRQ_THREADS), but there's only one semaphore \c softirq_sema. This'll be
 * fixed when we really support multiple threads.
 *
 * \todo Redesign for more than one thread.
 *
 * @{ */

/** softirq thread ids */
static l4thread_t softirq_tid[SOFTIRQ_THREADS];

/** softirq semaphore */
static l4semaphore_t softirq_sema = L4SEMAPHORE_LOCKED;

/** initialization flag */
static int _initialized = 0;

/** @} */
/** \name Softirqs
 *
 * <em>This is from include/linux/interrupt.h</em>
 *
 * Softirqs are multithreaded, not serialized BH-like activities. Several
 * softirqs may run on several CPUs simultaneously - no matter if they are of
 * the same type.
 *
 * Properties:
 *
 * - If raise_softirq() is called, then softirq is guarenteed to be executed on
 *   this CPU.
 * - On schedule() do_softirq() is called if any softirq is active on this CPU.
 * - Softirqs are not serialized in any way.
 *
 * Linux (2.4.20) has only 4 softirqs:
 *
 * - \c HI_SOFTIRQ
 * - \c NET_TX_SOFTIRQ and \c NET_RX_SOFTIRQ
 * - \c TASKLET_SOFTIRQ
 *
 * Relevant for Linux DDE are for now only the first and the latter - \c NET_*
 * softirqs allow transparent mutli-threading in Linux' network code. \c
 * HI_SOFTIRQ is for high priority bottom halves as \e old-style BHs and sound
 * related drivers. It triggers execution of tasklet_hi_action(). \c
 * TASKLET_SOFTIRQ runs lower priority bottom halves (e.g. in the console
 * subsystem). It triggers execution of tasklet_action().
 *
 * \todo only the implementation of tasklets is done in Linux DDE
 * @{ */

/** Raise softirq for dedicated CPU / handler thread.
 *
 * Must hold global lock when calling this.
 */
void __cpu_raise_softirq(unsigned cpu, int nr)
{
  l4semaphore_up(&softirq_sema);
}

/** Raise Softirq.
 * \ingroup mod_softirq
 *
 * \param nr  one of (HI_SOFTIRQ, NET_TX_SOFTIRQ, NET_RX_SOFTIRQ,
 *            TASKLET_SOFTIRQ)
 *
 * Grab global lock and raise softirq for a dedicated handler.
 */
void raise_softirq(int nr)
{
  /* original comment: I do not want to use atomic variables now, so that
     cli/sti */
  cli();
  __cpu_raise_softirq(0, nr);
  sti();
}

/** @} */
/** \name Tasklets
 *
 * <em>This is from kernel/%softirq.c and  include/linux/interrupt.h)</em>
 *
 * Tasklets are the multithreaded analogue of BHs.
 *
 * Main feature differing them of generic softirqs: one tasklet is running only
 * on one CPU simultaneously.
 *
 * Main feature differing them of BHs: different tasklets may be run
 * simultaneously on different CPUs.
 *
 * Properties:
 *
 * - If tasklet_schedule() is called, then tasklet is guaranteed to be executed
 *   on some cpu at least once after this.
 * - If the tasklet is already scheduled, but its excecution is still not
 *   started, it will be executed only once.
 * - If this tasklet is already running on another CPU (or schedule() is called
 *   from tasklet itself), it is rescheduled for later.
 * - Tasklet is strictly serialized wrt itself, but not wrt another
 *   tasklets. If client needs some intertask synchronization, he makes it with
 *   spinlocks.
 *
 * - these functions are thread-safe
 * - no assumption how much softirq threads
 * - one driver seldom uses more than 1 tasklet/bh therefore 1 tasklet thread
 *   is enough ?!
 *
 * Tasklet lists are CPU local in Linux and so tasklet related synchonization
 * is. This is not in Linux DDE - local_irq_disable()/enable() is not
 * sufficient. We use our cli()/sti() implementation.
 *
 * \todo Rethink tasklet_vec[1] for more than 1 softirq thread; so \c NR_CPUS
 * will become \c NR_SOFTIRQ_THREADS later
 *
 * @{ */

/** tasklet list head
 * 1-element vector (NR_CPUS==1) */
static struct tasklet_head tasklet_vec[NR_CPUS];

/** high prio tasklet list head
 * 1-element vector (NR_CPUS==1) */
static struct tasklet_head tasklet_hi_vec[NR_CPUS];

/** Tasklet Execution  */
static void tasklet_action(void)
{
  struct tasklet_struct *list;

  LOGd(DEBUG_SOFTIRQ, "low prio tasklet exec entrance");

  cli();
  list = tasklet_vec[0].list;
  tasklet_vec[0].list = NULL;
  sti();

  while (list != NULL)
    {
      struct tasklet_struct *t = list;

      list = list->next;

      if (tasklet_trylock(t))
        {
          if (atomic_read(&t->count) == 0)
            {
              clear_bit(TASKLET_STATE_SCHED, &t->state);

              t->func(t->data);
              tasklet_unlock(t);
              continue;
            }
          tasklet_unlock(t);
        }
      cli();
      t->next = tasklet_vec[0].list;
      tasklet_vec[0].list = t;
      __cpu_raise_softirq(0, TASKLET_SOFTIRQ);
      sti();
    }
}

/** High-Priority Tasklet Execution
 *
 * \return 0 on empty high-priority tasklet list
 */
static int tasklet_hi_action(void)
{
  struct tasklet_struct *list;

  if (!tasklet_hi_vec[0].list)
    /* no tasks */
    return 0;

  LOGd(DEBUG_SOFTIRQ, "hi prio tasklet exec entrance");

  cli();
  list = tasklet_hi_vec[0].list;
  tasklet_hi_vec[0].list = NULL;
  sti();

  while (list != NULL)
    {
      struct tasklet_struct *t = list;

      list = list->next;

      if (tasklet_trylock(t))
        {
          if (atomic_read(&t->count) == 0)
            {
              clear_bit(TASKLET_STATE_SCHED, &t->state);

              t->func(t->data);
              tasklet_unlock(t);
              continue;
            }
          tasklet_unlock(t);
        }
      cli();
      t->next = tasklet_hi_vec[0].list;
      tasklet_hi_vec[0].list = t;
      __cpu_raise_softirq(0, HI_SOFTIRQ);
      sti();
    }

  return !0;
}

/** Tasklet Initialization.
 * \ingroup mod_softirq
 *
 * \param t     tasklet struct that should be initialized
 * \param func  task of this deferred activity (handler function)
 * \param data  data cookie passed to handler
 */
void tasklet_init(struct tasklet_struct *t,
                  void (*func) (unsigned long), unsigned long data)
{
  t->func = func;
  t->data = data;
  t->state = 0;
  atomic_set(&t->count, 0);
}

/** Tasklet Termination.
 * \ingroup mod_softirq
 *
 * \param t  tasklet to be killed
 */
void tasklet_kill(struct tasklet_struct *t)
{
//      if (in_interrupt())
//              printf("Attempt to kill tasklet from interrupt\n");

  while (test_and_set_bit(TASKLET_STATE_SCHED, &t->state))
    {
#if 0   /* original implementation uses schedule() */
      current->state = TASK_RUNNING;
      do
        {
          current->policy |= SCHED_YIELD;
          schedule();
        }
      while (test_bit(TASKLET_STATE_SCHED, &t->state));
#else /* spin for tasklet while it is scheduled */
      do
        {
          /* release CPU on any way (like schedule() does in DDE) */
# if SCHED_YIELD_OPT
          l4thread_usleep(SCHED_YIELD_TO);
# else
          l4_yield();
# endif
        }
      while (test_bit(TASKLET_STATE_SCHED, &t->state));
#endif /* 0 */
    }
  tasklet_unlock_wait(t);
  clear_bit(TASKLET_STATE_SCHED, &t->state);
}

/** Schedule dedicated tasklet.
 * \ingroup mod_softirq
 *
 * \param t  tasklet to be scheduled for execution
 *
 * If tasklet is not already scheduled, grab global lock, enqueue as first in
 * global list, and raise softirq.
 */
void tasklet_schedule(struct tasklet_struct *t)
{
  if (!test_and_set_bit(TASKLET_STATE_SCHED, &t->state))
    {
      cli();
      t->next = tasklet_vec[0].list;
      tasklet_vec[0].list = t;

      /* raise softirq only on new 1st element */
      if (!t->next)
        __cpu_raise_softirq(0, TASKLET_SOFTIRQ);
      sti();
    }
}

/*****************************************************************************/
/** Schedule dedicated high-priority tasklet.
 * \ingroup mod_softirq
 *
 * \param t  high-priority tasklet to be scheduled for execution
 *
 * If tasklet is not already scheduled, grab global lock, enqueue as first in
 * global list, and raise softirq.
 */
/*****************************************************************************/
void tasklet_hi_schedule(struct tasklet_struct *t)
{
  if (!test_and_set_bit(TASKLET_STATE_SCHED, &t->state))
    {
      cli();
      t->next = tasklet_hi_vec[0].list;
      tasklet_hi_vec[0].list = t;

      /* raise softirq only on new 1st element */
      if (!t->next)
        __cpu_raise_softirq(0, HI_SOFTIRQ);
      sti();
    }
}

/** @} */
/** \name Old-style Bottom Halves and Task Queues
 *
 * <em>This is from kernel/%softirq.c</em>
 *
 * All bottom halves run as one tasklet so no two bottom halves can run
 * simultaneously.
 *
 * \todo Implement this if any useful driver needs it.
 *
 * \todo encapsulate #tqueue_lock like #tasklet_vec providing proper interface
 *
 * @{ */

/** protects tqueue list operation
 * <em>(from kernel/timer.c)</em> */
spinlock_t tqueue_lock = SPIN_LOCK_UNLOCKED;

/** Task Queue Execution
 * \ingroup mod_softirq
 *
 * Runs _all_ tasks currently in task queue \a list.
 */
void __run_task_queue(task_queue * list)
{
  struct list_head head, *next;
  unsigned long flags;

  spin_lock_irqsave(&tqueue_lock, flags);
  list_add(&head, list);
  list_del_init(list);
  spin_unlock_irqrestore(&tqueue_lock, flags);

  next = head.next;
  while (next != &head)
    {
      void (*f) (void *);
      struct tq_struct *p;
      void *data;

      p = list_entry(next, struct tq_struct, list);
      next = next->next;
      f = p->routine;
      data = p->data;
      wmb();
      p->sync = 0;
      if (f)
        f(data);
    }
}

/** @} */
/** Linux DDE Softirq Thread(s)
 *
 * \param num   number of softirq thread (for now always 0)
 *
 * \krishna call softirq_action functions directly; later open_softirq
 * implementation and call via softirq_vec[]
 *
 * \todo priorities
 */
static void dde_softirq_thread(int num)
{
  softirq_tid[num] = l4thread_myself();

  if (l4dde_process_add_worker())
      Panic("l4dde_process_add_worker() failed");

  ++local_bh_count(smp_processor_id());

  /* we are up */
  if (l4thread_started(NULL)<0)
    Panic("softirq thread startup failed!");

  LOGd(DEBUG_MSG, "dde_softirq_thread[%d] "l4util_idfmt" running.", num,
       l4util_idstr(l4thread_l4_id(l4thread_myself())));

  /* softirq loop */
  while (1)
    {
      /* softirq _consumer_ */
      l4semaphore_down(&softirq_sema);

      /* low-priority tasks only if no high-priority available */
      if (!tasklet_hi_action())
        tasklet_action();
    }
}

/** Initalize Softirq Thread(s)
 * \ingroup mod_softirq
 *
 * \return 0 on success; negative error code otherwise
 *
 * All threads for deferred activities are created on initialization.
 *
 * \todo configuration with more (than 1) threads
 */
int l4dde_softirq_init()
{
#if !(SOFTIRQ_THREADS == 1)
#error SOFTIRQ_THREADS has to be 1
#else
  int err;

  if (_initialized)
    return 0;

  /* create softirq thread */
  err = l4thread_create_long(L4THREAD_INVALID_ID,
                             (l4thread_fn_t) dde_softirq_thread,
                             ".softirq",
                             L4THREAD_INVALID_SP,
                             L4THREAD_DEFAULT_SIZE,
                             L4THREAD_DEFAULT_PRIO,
                             (void *) 0,
                             L4THREAD_CREATE_SYNC);

  if (err < 0)
    return err;

  ++_initialized;
  return 0;
#endif
}
