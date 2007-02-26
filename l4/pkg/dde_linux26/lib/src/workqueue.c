/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux26/lib/src/workqueue.c
 * \brief  Workqueue implementation
 *
 * \author Marek Menzer <mm19@os.inf.tu-dresden.de>
 *
 * Generic mechanism for defining kernel helper threads for running
 * arbitrary tasks in process context.
 *
 * Started by Ingo Molnar, Copyright (C) 2002
 *
 * Derived from the taskqueue/keventd code by:
 *
 *   David Woodhouse <dwmw2@redhat.com>
 *   Andrew Morton <andrewm@uow.edu.au>
 *   Kai Petzke <wpp@marie.physik.tu-berlin.de>
 *   Theodore Ts'o <tytso@mit.edu>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/signal.h>
#include <linux/completion.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/notifier.h>
#include <linux/kthread.h>

/*
 * The per-CPU workqueue (if single thread, we always use cpu 0's)
 *
 * The sequence counters are for flush_scheduled_work().  It wants to wait
 * until until all currently-scheduled works are completed, but it doesn't
 * want to be livelocked by new, incoming ones.  So it waits until
 * remove_sequence is >= the insert_sequence which pertained when
 * flush_scheduled_work() was called.
 */
struct cpu_workqueue_struct {

	spinlock_t lock;

	long remove_sequence;	/* Least-recently added (next to run) */
	long insert_sequence;	/* Next to add */

	struct list_head worklist;
	wait_queue_head_t more_work;
	wait_queue_head_t work_done;

	struct workqueue_struct *wq;
	task_t *thread;

	int run_depth;		/* Detect run_workqueue() recursion depth */
} ____cacheline_aligned;

/*
 * The externally visible workqueue abstraction is an array of
 * per-CPU workqueues:
 */
struct workqueue_struct {
	struct cpu_workqueue_struct cpu_wq[NR_CPUS];
	const char *name;
	struct list_head list;	/* Empty if single thread*/
};

/* All the per-cpu workqueues on the system, for hotplug cpu to add/remove
   threads to each one as cpus come/go. */
static spinlock_t workqueue_lock = SPIN_LOCK_UNLOCKED;
static LIST_HEAD(workqueues);

/* If it's single threaded, it isn't in the list of workqueues. */
static inline int is_single_threaded(struct workqueue_struct *wq)
{
	return list_empty(&wq->list);
}

/* Preempt must be disabled. */
static void __queue_work(struct cpu_workqueue_struct *cwq,
			 struct work_struct *work)
{
	unsigned long flags;

	spin_lock_irqsave(&cwq->lock, flags);
	work->wq_data = cwq;
	list_add_tail(&work->entry, &cwq->worklist);
	cwq->insert_sequence++;
	wake_up(&cwq->more_work);
	spin_unlock_irqrestore(&cwq->lock, flags);
}

/*
 * Queue work on a workqueue. Return non-zero if it was successfully
 * added.
 *
 * We queue the work to the CPU it was submitted, but there is no
 * guarantee that it will be processed by that CPU.
 */
int fastcall queue_work(struct workqueue_struct *wq, struct work_struct *work)
{
	int ret = 0, cpu = get_cpu();

	if (!test_and_set_bit(0, &work->pending)) {
		if (unlikely(is_single_threaded(wq)))
			cpu = 0;
		BUG_ON(!list_empty(&work->entry));
		__queue_work(wq->cpu_wq + cpu, work);
		ret = 1;
	}
	put_cpu();
	return ret;
}

static void delayed_work_timer_fn(unsigned long __data)
{
	struct work_struct *work = (struct work_struct *)__data;
	struct workqueue_struct *wq = work->wq_data;
	int cpu = smp_processor_id();
	
	if (unlikely(is_single_threaded(wq)))
		cpu = 0;

	__queue_work(wq->cpu_wq + cpu, work);
}

int fastcall queue_delayed_work(struct workqueue_struct *wq,
			struct work_struct *work, unsigned long delay)
{
	int ret = 0;
	struct timer_list *timer = &work->timer;

	if (!test_and_set_bit(0, &work->pending)) {
		BUG_ON(timer_pending(timer));
		BUG_ON(!list_empty(&work->entry));

		/* This stores wq for the moment, for the timer_fn */
		work->wq_data = wq;
		timer->expires = jiffies + delay;
		timer->data = (unsigned long)work;
		timer->function = delayed_work_timer_fn;
		add_timer(timer);
		ret = 1;
	}
	return ret;
}

static inline void run_workqueue(struct cpu_workqueue_struct *cwq)
{
	unsigned long flags;

	/*
	 * Keep taking off work from the queue until
	 * done.
	 */
	spin_lock_irqsave(&cwq->lock, flags);
	cwq->run_depth++;
	if (cwq->run_depth > 3) {
		/* morton gets to eat his hat */
		printk("%s: recursion depth exceeded: %d\n",
			__FUNCTION__, cwq->run_depth);
//		dump_stack();
	}
	while (!list_empty(&cwq->worklist)) {
		struct work_struct *work = list_entry(cwq->worklist.next,
						struct work_struct, entry);
		void (*f) (void *) = work->func;
		void *data = work->data;

		list_del_init(cwq->worklist.next);
		spin_unlock_irqrestore(&cwq->lock, flags);

		BUG_ON(work->wq_data != cwq);
		clear_bit(0, &work->pending);
		f(data);

		spin_lock_irqsave(&cwq->lock, flags);
		cwq->remove_sequence++;
		wake_up(&cwq->work_done);
	}
	cwq->run_depth--;
	spin_unlock_irqrestore(&cwq->lock, flags);
}

static int worker_thread(void *__cwq)
{
	struct cpu_workqueue_struct *cwq = __cwq;
	DECLARE_WAITQUEUE(wait, current);

	current->flags |= PF_NOFREEZE;

//	set_user_nice(current, -10);

	while (!kthread_should_stop()) {
		set_task_state(current, TASK_INTERRUPTIBLE);

		add_wait_queue(&cwq->more_work, &wait);
		if (list_empty(&cwq->worklist))
			schedule();
		else
			set_task_state(current, TASK_RUNNING);
		remove_wait_queue(&cwq->more_work, &wait);

		if (!list_empty(&cwq->worklist))
			run_workqueue(cwq);
	}
	return 0;
}

/*
 * flush_workqueue - ensure that any scheduled work has run to completion.
 *
 * Forces execution of the workqueue and blocks until its completion.
 * This is typically used in driver shutdown handlers.
 *
 * This function will sample each workqueue's current insert_sequence number and
 * will sleep until the head sequence is greater than or equal to that.  This
 * means that we sleep until all works which were queued on entry have been
 * handled, but we are not livelocked by new incoming ones.
 *
 * This function used to run the workqueues itself.  Now we just wait for the
 * helper threads to do it.
 */
void fastcall flush_workqueue(struct workqueue_struct *wq)
{
	struct cpu_workqueue_struct *cwq;
	int cpu;

	might_sleep();

	lock_cpu_hotplug();
	for_each_online_cpu(cpu) {
		DEFINE_WAIT(wait);
		long sequence_needed;

		if (is_single_threaded(wq))
			cwq = wq->cpu_wq + 0; /* Always use cpu 0's area. */
		else
			cwq = wq->cpu_wq + cpu;

		if (cwq->thread == current) {
			/*
			 * Probably keventd trying to flush its own queue.
			 * So simply run it by hand rather than deadlocking.
			 */
			run_workqueue(cwq);
			continue;
		}
		spin_lock_irq(&cwq->lock);
		sequence_needed = cwq->insert_sequence;

		while (sequence_needed - cwq->remove_sequence > 0) {
			prepare_to_wait(&cwq->work_done, &wait,
					TASK_UNINTERRUPTIBLE);
			spin_unlock_irq(&cwq->lock);
			schedule();
			spin_lock_irq(&cwq->lock);
		}
		finish_wait(&cwq->work_done, &wait);
		spin_unlock_irq(&cwq->lock);
	}
	unlock_cpu_hotplug();
}

static struct task_struct *create_workqueue_thread(struct workqueue_struct *wq,
						   int cpu)
{
	struct cpu_workqueue_struct *cwq = wq->cpu_wq + cpu;
	struct task_struct *p;

	spin_lock_init(&cwq->lock);
	cwq->wq = wq;
	cwq->thread = NULL;
	cwq->insert_sequence = 0;
	cwq->remove_sequence = 0;
	INIT_LIST_HEAD(&cwq->worklist);
	init_waitqueue_head(&cwq->more_work);
	init_waitqueue_head(&cwq->work_done);

	if (is_single_threaded(wq))
		p = kthread_create(worker_thread, cwq, "%s", wq->name);
	else
		p = kthread_create(worker_thread, cwq, "%s/%d", wq->name, cpu);
	if (IS_ERR(p))
		return NULL;
	cwq->thread = p;
	return p;
}

struct workqueue_struct *__create_workqueue(const char *name,
					    int singlethread)
{
	int cpu, destroy = 0;
	struct workqueue_struct *wq;
	struct task_struct *p;

	BUG_ON(strlen(name) > 10);

	wq = kmalloc(sizeof(*wq), GFP_KERNEL);
	if (!wq)
		return NULL;
	memset(wq, 0, sizeof(*wq));

	wq->name = name;
	/* We don't need the distraction of CPUs appearing and vanishing. */
	lock_cpu_hotplug();
	if (singlethread) {
		INIT_LIST_HEAD(&wq->list);
		p = create_workqueue_thread(wq, 0);
		if (!p)
			destroy = 1;
		else
			wake_up_process(p);
	} else {
		spin_lock(&workqueue_lock);
		list_add(&wq->list, &workqueues);
		spin_unlock_irq(&workqueue_lock);
		for_each_online_cpu(cpu) {
			p = create_workqueue_thread(wq, cpu);
			if (p) {
				kthread_bind(p, cpu);
				wake_up_process(p);
			} else
				destroy = 1;
		}
	}

	/*
	 * Was there any error during startup? If yes then clean up:
	 */
	if (destroy) {
		destroy_workqueue(wq);
		wq = NULL;
	}
	unlock_cpu_hotplug();
	return wq;
}

static void cleanup_workqueue_thread(struct workqueue_struct *wq, int cpu)
{
	struct cpu_workqueue_struct *cwq;
	unsigned long flags;
	struct task_struct *p;

	cwq = wq->cpu_wq + cpu;
	spin_lock_irqsave(&cwq->lock, flags);
	p = cwq->thread;
	cwq->thread = NULL;
	spin_unlock_irqrestore(&cwq->lock, flags);
	if (p)
		kthread_stop(p);
}

void destroy_workqueue(struct workqueue_struct *wq)
{
	int cpu;

	flush_workqueue(wq);

	/* We don't need the distraction of CPUs appearing and vanishing. */
	lock_cpu_hotplug();
	if (is_single_threaded(wq))
		cleanup_workqueue_thread(wq, 0);
	else {
		for_each_online_cpu(cpu)
			cleanup_workqueue_thread(wq, cpu);
		spin_lock(&workqueue_lock);
		list_del(&wq->list);
		spin_unlock_irq(&workqueue_lock);
	}
	unlock_cpu_hotplug();
	kfree(wq);
}

static struct workqueue_struct *keventd_wq;

int fastcall schedule_work(struct work_struct *work)
{
	return queue_work(keventd_wq, work);
}

int fastcall schedule_delayed_work(struct work_struct *work, unsigned long delay)
{
	return queue_delayed_work(keventd_wq, work, delay);
}

void flush_scheduled_work(void)
{
	flush_workqueue(keventd_wq);
}

int keventd_up(void)
{
	return keventd_wq != NULL;
}

int current_is_keventd(void)
{
	struct cpu_workqueue_struct *cwq;
	int cpu = smp_processor_id();	/* preempt-safe: keventd is per-cpu */
	int ret = 0;

	BUG_ON(!keventd_wq);

	cwq = keventd_wq->cpu_wq + cpu;
	if (current == cwq->thread)
		ret = 1;

	return ret;

}

void l4dde_workqueues_init(void)
{
	keventd_wq = create_workqueue("events");
	BUG_ON(!keventd_wq);
}
