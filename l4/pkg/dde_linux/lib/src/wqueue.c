/* $Id$ */
/*****************************************************************************/
/**
 * \file	dde_linux/lib/src/wqueue.c
 *
 * \brief	Wait Queues
 *
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* L4 */
#include <l4/dde_linux/dde.h>

/* Linux */
#include <asm/system.h>		/* local_irq_save and friends */
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
void add_wait_queue(wait_queue_head_t * q, wait_queue_t * wait)
{
  unsigned long flags;

  wq_write_lock_irqsave(&q->lock, flags);
  wait->flags = 0;
  __add_wait_queue(q, wait);
  wq_write_unlock_irqrestore(&q->lock, flags);
}

/*****************************************************************************/
/** Enqueue process in wait queue (exclusive flag set)
 * \ingroup mod_proc */
/*****************************************************************************/
void add_wait_queue_exclusive(wait_queue_head_t * q, wait_queue_t * wait)
{
  unsigned long flags;

  wq_write_lock_irqsave(&q->lock, flags);
  wait->flags = WQ_FLAG_EXCLUSIVE;
  __add_wait_queue_tail(q, wait);
  wq_write_unlock_irqrestore(&q->lock, flags);
}

/*****************************************************************************/
/** Dequeue process from wait queue
 * \ingroup mod_proc */
/*****************************************************************************/
void remove_wait_queue(wait_queue_head_t * q, wait_queue_t * wait)
{
  unsigned long flags;

  wq_write_lock_irqsave(&q->lock, flags);
  __remove_wait_queue(q, wait);
  wq_write_unlock_irqrestore(&q->lock, flags);
}

/** @} */
