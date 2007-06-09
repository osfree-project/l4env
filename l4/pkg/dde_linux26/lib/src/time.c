/* $Id$ */
/*****************************************************************************/
/**
 * \file	dde_linux26/lib/src/time.c
 *
 * \brief	Time and Timers
 *
 * \author	Marek Menzer <mm19@os.inf.tu-dresden.de>
 *
 * Most code from Linux. Idea for emulation by 
 * Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
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
/*****************************************************************************/

/* L4 */
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/thread/thread.h>
#include <l4/lock/lock.h>
#include <l4/util/util.h>

#include <l4/dde_linux/dde.h>

/* Linux */
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/list.h>

/* local */
#include "internal.h"
#include "__config.h"
#include "__macros.h"

/*****************************************************************************/
/**
 * \name Linux Timer Handling
 *
 * <em>This is from kernel/timer.c</em>
 * @{ */
/*****************************************************************************/
/** timer thread id */
static l4thread_t timer_tid = L4THREAD_INVALID_ID;

/** initialization flag */
static int _initialized = 0;

/*
 * per-CPU timer vector definitions:
 */
#define TVN_BITS 6
#define TVR_BITS 8
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_MASK (TVN_SIZE - 1)
#define TVR_MASK (TVR_SIZE - 1)

typedef struct tvec_s {
	struct list_head vec[TVN_SIZE];
} tvec_t;

typedef struct tvec_root_s {
	struct list_head vec[TVR_SIZE];
} tvec_root_t;

struct tvec_t_base_s {
	spinlock_t lock;
	unsigned long timer_jiffies;
	struct timer_list *running_timer;
	tvec_root_t tv1;
	tvec_t tv2;
	tvec_t tv3;
	tvec_t tv4;
	tvec_t tv5;
} ____cacheline_aligned_in_smp;

typedef struct tvec_t_base_s tvec_base_t;

static tvec_base_t tvec_bases;

static inline void set_running_timer(tvec_base_t *base,
					struct timer_list *timer)
{
}

static void check_timer_failed(struct timer_list *timer)
{
	static int whine_count;
	if (whine_count < 16) {
		whine_count++;
		printk("Uninitialised timer!\n");
		printk("This is just a warning.  Your computer is OK\n");
		printk("function=0x%p, data=0x%lx\n",
			timer->function, timer->data);
//		dump_stack();
	}
	/*
	 * Now fix it up
	 */
	spin_lock_init(&timer->lock);
	timer->magic = TIMER_MAGIC;
}

static inline void check_timer(struct timer_list *timer)
{
	if (timer->magic != TIMER_MAGIC)
		check_timer_failed(timer);
}

/*****************************************************************************/
/** Restart Timer Thread
 */
/*****************************************************************************/
static inline void
__restart_timer_thread(void)
{
  l4_threadid_t timer_l4id;
  l4_msgdope_t result;
  int err;

  timer_l4id = l4thread_l4_id(timer_tid);

#if 0
  /* do not notify myself about list update (it's checked later in timer
    loop) */
  if (thread_equal(timer_l4id, l4_myself()))
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
	LOG_Error("IPC error 0x%02lx in __restart()", L4_IPC_ERROR(result));
    }
  while (err);
}

/*****************************************************************************/
/** Adding One-Shot Timer To List Helper
 */
/*****************************************************************************/
static void internal_add_timer(tvec_base_t *base, struct timer_list *timer)
{
	unsigned long expires = timer->expires;
	unsigned long idx = expires - base->timer_jiffies;
	struct list_head *vec;

	if (idx < TVR_SIZE) {
		int i = expires & TVR_MASK;
		vec = base->tv1.vec + i;
	} else if (idx < 1 << (TVR_BITS + TVN_BITS)) {
		int i = (expires >> TVR_BITS) & TVN_MASK;
		vec = base->tv2.vec + i;
	} else if (idx < 1 << (TVR_BITS + 2 * TVN_BITS)) {
		int i = (expires >> (TVR_BITS + TVN_BITS)) & TVN_MASK;
		vec = base->tv3.vec + i;
	} else if (idx < 1 << (TVR_BITS + 3 * TVN_BITS)) {
		int i = (expires >> (TVR_BITS + 2 * TVN_BITS)) & TVN_MASK;
		vec = base->tv4.vec + i;
	} else if ((signed long) idx < 0) {
		/*
		 * Can happen if you add a timer with expires == jiffies,
		 * or you set a timer to go off in the past
		 */
		vec = base->tv1.vec + (base->timer_jiffies & TVR_MASK);
	} else {
		int i;
		/* If the timeout is larger than 0xffffffff on 64-bit
		 * architectures then we use the maximum timeout:
		 */
		if (idx > 0xffffffffUL) {
			idx = 0xffffffffUL;
			expires = idx + base->timer_jiffies;
		}
		i = (expires >> (TVR_BITS + 3 * TVN_BITS)) & TVN_MASK;
		vec = base->tv5.vec + i;
	}
	/*
	 * Timers are FIFO:
	 */
	list_add_tail(&timer->entry, vec);
/*
#if DEBUG_TIMER
  DMSG("ADDED timer (current list follows)\n");
  __list_for_each(tmp, &timer_list)
  {
    struct timer_list *tp = list_entry(tmp, struct timer_list, entry);
    DMSG("  [%p] expires %lu, data = %lu\n", tmp, tp->expires, tp->data);
  }
#endif
*/
}

/*****************************************************************************/
/** Modifying One-Shot Timer
 */
/*****************************************************************************/
int __mod_timer(struct timer_list *timer, unsigned long expires)
{
	tvec_base_t *old_base, *new_base;
	unsigned long flags;
	int ret = 0;

	BUG_ON(!timer->function);

	check_timer(timer);

	spin_lock_irqsave(&timer->lock, flags);
	new_base = &tvec_bases;
repeat:
	old_base = timer->base;

	/*
	 * Prevent deadlocks via ordering by old_base < new_base.
	 */
	if (old_base && (new_base != old_base)) {
		if (old_base < new_base) {
			spin_lock(&new_base->lock);
			spin_lock(&old_base->lock);
		} else {
			spin_lock(&old_base->lock);
			spin_lock(&new_base->lock);
		}
		/*
		 * The timer base might have been cancelled while we were
		 * trying to take the lock(s):
		 */
		if (timer->base != old_base) {
			spin_unlock(&new_base->lock);
			spin_unlock(&old_base->lock);
			goto repeat;
		}
	} else {
		spin_lock(&new_base->lock);
		if (timer->base != old_base) {
			spin_unlock(&new_base->lock);
			goto repeat;
		}
	}

	/*
	 * Delete the previous timeout (if there was any), and install
	 * the new one:
	 */
	if (old_base) {
		list_del(&timer->entry);
		ret = 1;
	}
	timer->expires = expires;
	internal_add_timer(new_base, timer);
	timer->base = new_base;

	if (old_base && (new_base != old_base))
		spin_unlock(&old_base->lock);
	spin_unlock(&new_base->lock);
	spin_unlock_irqrestore(&timer->lock, flags);

	return ret;
}

/*****************************************************************************/
/***
 * add_timer_on - start a timer on a particular CPU
 * @timer: the timer to be added
 * @cpu: the CPU to start it on
 *
 * This is not very scalable on SMP. Double adds are not possible.
 */
/*****************************************************************************/
void add_timer_on(struct timer_list *timer, int cpu)
{
	tvec_base_t *base = &tvec_bases;
  	unsigned long flags;
  
  	BUG_ON(timer_pending(timer) || !timer->function);

	check_timer(timer);

	spin_lock_irqsave(&base->lock, flags);
	internal_add_timer(base, timer);
	timer->base = base;
	spin_unlock_irqrestore(&base->lock, flags);
}

/*****************************************************************************/
/**
 * mod_timer - modify a timer's timeout
 * @timer: the timer to be modified
 *
 * mod_timer is a more efficient way to update the expire field of an
 * active timer (if the timer is inactive it will be activated)
 *
 * mod_timer(timer, expires) is equivalent to:
 *
 *     del_timer(timer); timer->expires = expires; add_timer(timer);
 *
 * Note that if there are multiple unserialized concurrent users of the
 * same timer, then mod_timer() is the only safe way to modify the timeout,
 * since add_timer() cannot modify an already running timer.
 *
 * The function returns whether it has modified a pending timer or not.
 * (ie. mod_timer() of an inactive timer returns 0, mod_timer() of an
 * active timer returns 1.)
 */
/*****************************************************************************/
int mod_timer(struct timer_list *timer, unsigned long expires)
{
	BUG_ON(!timer->function);

	check_timer(timer);

	/*
	 * This is a common optimization triggered by the
	 * networking code - if the timer is re-modified
	 * to be the same thing then just return:
	 */
	if (timer->expires == expires && timer_pending(timer))
		return 1;

	return __mod_timer(timer, expires);
}

/*****************************************************************************/
/***
 * del_timer - deactive a timer.
 * @timer: the timer to be deactivated
 *
 * del_timer() deactivates a timer - this works on both active and inactive
 * timers.
 *
 * The function returns whether it has deactivated a pending timer or not.
 * (ie. del_timer() of an inactive timer returns 0, del_timer() of an
 * active timer returns 1.)
 */
/*****************************************************************************/
int del_timer(struct timer_list *timer)
{
	unsigned long flags;
	tvec_base_t *base;

	check_timer(timer);

repeat:
 	base = timer->base;
	if (!base)
		return 0;
	spin_lock_irqsave(&base->lock, flags);
	if (base != timer->base) {
		spin_unlock_irqrestore(&base->lock, flags);
		goto repeat;
	}
	list_del(&timer->entry);
	timer->base = NULL;
	spin_unlock_irqrestore(&base->lock, flags);

	return 1;
}

/*****************************************************************************/
/** Timer Sleep Implementation
 *
 * \param  to		IPC timeout
 *
 * \return 1 on timed out IPC (0 = message received and indicates \c
 * __restart)
 *
 * Has to be called holding the timerlist lock.
 */
/*****************************************************************************/
static inline int
__timer_sleep(l4_timeout_t to)
{
  l4_threadid_t me;
  l4_threadid_t any;
  l4_umword_t dummy;
  l4_msgdope_t result;
  int err;

  me = l4thread_l4_id(timer_tid);

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

  /* done */
  return (err ? 1 : 0);
}

/** @} */

static int cascade(tvec_base_t *base, tvec_t *tv, int index)
{
	/* cascade all the timers from tv up one level */
	struct list_head *head, *curr;

	head = tv->vec + index;
	curr = head->next;
	/*
	 * We are removing _all_ timers from the list, so we don't  have to
	 * detach them individually, just clear the list afterwards.
	 */
	while (curr != head) {
		struct timer_list *tmp;

		tmp = list_entry(curr, struct timer_list, entry);
		BUG_ON(tmp->base != base);
		curr = curr->next;
		internal_add_timer(base, tmp);
	}
	INIT_LIST_HEAD(head);

	return index;
}

/***
 * __run_timers - run all expired timers (if any) on this CPU.
 * @base: the timer vector to be processed.
 *
 * This function cascades all vectors and executes all expired timer
 * vectors.
 */
#define INDEX(N) (base->timer_jiffies >> (TVR_BITS + N * TVN_BITS)) & TVN_MASK

static inline void __run_timers(tvec_base_t *base)
{
	struct timer_list *timer;

	spin_lock_irq(&base->lock);
	while (time_after_eq(jiffies, base->timer_jiffies)) {
		struct list_head work_list = LIST_HEAD_INIT(work_list);
		struct list_head *head = &work_list;
 		int index = base->timer_jiffies & TVR_MASK;
 
		/*
		 * Cascade timers:
		 */
		if (!index &&
			(!cascade(base, &base->tv2, INDEX(0))) &&
				(!cascade(base, &base->tv3, INDEX(1))) &&
					!cascade(base, &base->tv4, INDEX(2)))
			cascade(base, &base->tv5, INDEX(3));
		++base->timer_jiffies; 
		list_splice_init(base->tv1.vec + index, &work_list);
repeat:
		if (!list_empty(head)) {
			void (*fn)(unsigned long);
			unsigned long data;

			timer = list_entry(head->next,struct timer_list,entry);
 			fn = timer->function;
 			data = timer->data;

			list_del(&timer->entry);
			set_running_timer(base, timer);
			smp_wmb();
			timer->base = NULL;
			spin_unlock_irq(&base->lock);
			fn(data);
			spin_lock_irq(&base->lock);
			goto repeat;
		}
	}
	set_running_timer(base, NULL);
	spin_unlock_irq(&base->lock);
}

/*
 * This function runs timers and the timer-tq in bottom half context.
 */
static void run_timer_softirq(void)
{
	tvec_base_t *base = &tvec_bases;

	if (time_after_eq(jiffies, base->timer_jiffies))
		__run_timers(base);
}
/*****************************************************************************/
/** Linux DDE Timer Thread
 *
 * We sleep for the most time: until next timer or L4_IPC_NEVER. If timer
 * expires release lock and do timer function.
 *
 * \todo priorities
 */
/*****************************************************************************/
static void dde_timer_thread(void)
{
  l4_timeout_t to;
  l4_timeout_s to_send;
	int to_us;

  timer_tid = l4thread_myself();

  if (l4thread_started(NULL))
    goto ret;

  LOGd(DEBUG_MSG, "dde_timer_thread "l4util_idfmt" running.",
       l4util_idstr(l4thread_l4_id(l4thread_myself())));

  to_us = 1000000 / HZ;
  to_send =  l4util_micros2l4to(to_us);
	to = l4_timeout(L4_IPC_TIMEOUT_NEVER, to_send);

  /* timer loop */
  while (1)
    {
      LOGd(DEBUG_TIMER, "begin of timer_loop");
      /* wait for timeout or __restart */
      __timer_sleep(to);
      run_timer_softirq();
    }

 ret:
  /* that should never happen */
  Panic("left timer loop!");
  l4thread_exit();
}

/*****************************************************************************/
/** Initialize Timer Thread
 * \ingroup mod_time
 *
 * \return 0 on success; negative error code otherwise
 *
 * One timer thread is created on initialization.
 */
/*****************************************************************************/
int l4dde_time_init()
{
  int err;
  int j;
  tvec_base_t *base = &tvec_bases;

  spin_lock_init(&base->lock);
  for (j = 0; j < TVN_SIZE; j++) {
    INIT_LIST_HEAD(base->tv5.vec + j);
    INIT_LIST_HEAD(base->tv4.vec + j);
    INIT_LIST_HEAD(base->tv3.vec + j);
    INIT_LIST_HEAD(base->tv2.vec + j);
  }
  for (j = 0; j < TVR_SIZE; j++)
    INIT_LIST_HEAD(base->tv1.vec + j);

  base->timer_jiffies = jiffies;

  /* create timer thread */
  err = l4thread_create((l4thread_fn_t) dde_timer_thread,
			0, L4THREAD_CREATE_SYNC);

  if (err < 0)
    return err;

  ++_initialized;
  return 0;
}
