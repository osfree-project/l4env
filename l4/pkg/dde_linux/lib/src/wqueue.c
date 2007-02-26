/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux/lib/src/wqueue.c
 * \brief  Wait Queues
 *
 * \date   08/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 */
#include <l4/dde_linux/dde.h>

/* Linux */
#include <asm/system.h>  /* local_irq_save and friends */
#include <linux/wait.h>
#include <linux/sched.h>

#include "fastcall.h"

/** \name Wait Queue - List Manipulation
 *
 * <em>This is from kernel/fork.c</em>
 * @{ */

/** Enqueue process in wait queue
 * \ingroup mod_proc */
void FASTCALL(add_wait_queue(wait_queue_head_t * q, wait_queue_t * wait))
{
  unsigned long flags;

  wq_write_lock_irqsave(&q->lock, flags);
  wait->flags = 0;
  __add_wait_queue(q, wait);
  wq_write_unlock_irqrestore(&q->lock, flags);
}

/** Enqueue process in wait queue (exclusive flag set)
 * \ingroup mod_proc */
void FASTCALL(add_wait_queue_exclusive(wait_queue_head_t * q, wait_queue_t * wait))
{
  unsigned long flags;

  wq_write_lock_irqsave(&q->lock, flags);
  wait->flags = WQ_FLAG_EXCLUSIVE;
  __add_wait_queue_tail(q, wait);
  wq_write_unlock_irqrestore(&q->lock, flags);
}

/** Dequeue process from wait queue
 * \ingroup mod_proc */
void FASTCALL(remove_wait_queue(wait_queue_head_t * q, wait_queue_t * wait))
{
  unsigned long flags;

  wq_write_lock_irqsave(&q->lock, flags);
  __remove_wait_queue(q, wait);
  wq_write_unlock_irqrestore(&q->lock, flags);
}

/** @} */
