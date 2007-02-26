/* $Id$ */
/*****************************************************************************/
/**
 * \file	dde_linux26/lib/src/wqueue.c
 *
 * \brief	Wait Queues
 *
 * \author	Marek Menzer <mm19@os.inf.tu-dresden.de>
 *
 * Partly based on dde_linux version by
 * Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4 */
#include <l4/dde_linux/dde.h>

/* Linux */
#include <asm/system.h>		/* local_irq_save and friends */
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/sched.h>

/*****************************************************************************/
/**
 * \name Wait Queue - List Manipulation
 *
 * <em>This is from kernel/fork.c</em>
 * @{ */
/*****************************************************************************/

/*****************************************************************************/
/** Enqueue process in wait queue
 * \ingroup mod_proc */
/*****************************************************************************/
void fastcall add_wait_queue(wait_queue_head_t * q, wait_queue_t * wait)
{
  unsigned long flags;

  spin_lock_irqsave(&q->lock, flags);
  wait->flags = 0;
  __add_wait_queue(q, wait);
  spin_unlock_irqrestore(&q->lock, flags);
}

/*****************************************************************************/
/** Enqueue process in wait queue (exclusive flag set)
 * \ingroup mod_proc */
/*****************************************************************************/
void fastcall add_wait_queue_exclusive(wait_queue_head_t * q, wait_queue_t * wait)
{
  unsigned long flags;

  spin_lock_irqsave(&q->lock, flags);
  wait->flags = WQ_FLAG_EXCLUSIVE;
  __add_wait_queue_tail(q, wait);
  spin_unlock_irqrestore(&q->lock, flags);
}

/*****************************************************************************/
/** Dequeue process from wait queue
 * \ingroup mod_proc */
/*****************************************************************************/
void fastcall remove_wait_queue(wait_queue_head_t * q, wait_queue_t * wait)
{
  unsigned long flags;

  spin_lock_irqsave(&q->lock, flags);
  __remove_wait_queue(q, wait);
  spin_unlock_irqrestore(&q->lock, flags);
}

void fastcall prepare_to_wait(wait_queue_head_t *q, wait_queue_t *wait, int state)
{
	unsigned long flags;

	__set_current_state(state);
	spin_lock_irqsave(&q->lock, flags);
	wait->flags = 0;
	if (list_empty(&wait->task_list))
		__add_wait_queue(q, wait);
	spin_unlock_irqrestore(&q->lock, flags);
}

void fastcall
prepare_to_wait_exclusive(wait_queue_head_t *q, wait_queue_t *wait, int state)
{
	unsigned long flags;

	__set_current_state(state);
	spin_lock_irqsave(&q->lock, flags);
	wait->flags = WQ_FLAG_EXCLUSIVE;
	if (list_empty(&wait->task_list))
		__add_wait_queue_tail(q, wait);
	spin_unlock_irqrestore(&q->lock, flags);
}

void fastcall finish_wait(wait_queue_head_t *q, wait_queue_t *wait)
{
	unsigned long flags;

	__set_current_state(TASK_RUNNING);
	if (!list_empty(&wait->task_list)) {
		spin_lock_irqsave(&q->lock, flags);
		list_del_init(&wait->task_list);
		spin_unlock_irqrestore(&q->lock, flags);
	}
}

int autoremove_wake_function(wait_queue_t *wait, unsigned mode, int sync)
{
	int ret = default_wake_function(wait, mode, sync);

	if (ret)
		list_del_init(&wait->task_list);
	return ret;
}
