/*
 * \brief   DDEUSB client library - IPC wrapper and D_URB management
 * \date    2009-04-07
 * \author  Dirk Vogt <dvogt@os.inf.tu-dresden.de>
 */

/*
 * This file is part of the DDEUSB package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <linux/mm.h>
#include <linux/sched.h>

#define	SLEEP_ON_VAR					\
	unsigned long flags;				\
	wait_queue_t wait;				\
	init_waitqueue_entry(&wait, current);

#define SLEEP_ON_HEAD					\
	spin_lock_irqsave(&q->lock,flags);		\
	__add_wait_queue(q, &wait);			\
	spin_unlock(&q->lock);

#define	SLEEP_ON_TAIL					\
	spin_lock_irq(&q->lock);			\
	__remove_wait_queue(q, &wait);			\
	spin_unlock_irqrestore(&q->lock, flags);

void fastcall __sched interruptible_sleep_on(wait_queue_head_t *q)
{
	SLEEP_ON_VAR

	current->state = TASK_INTERRUPTIBLE;

	SLEEP_ON_HEAD
	schedule();
	SLEEP_ON_TAIL
}

/******************************************************************************
*       mm/memory.c                                                           *
******************************************************************************/

/*
 *  * Map a vmalloc()-space virtual address to the physical page frame number.
 *   */
unsigned long vmalloc_to_pfn(void * vmalloc_addr)
{
	return page_to_pfn(vmalloc_to_page(vmalloc_addr));
}

struct page * vmalloc_to_page(void *addr)
{
	return virt_to_page(addr);
}


void *vmalloc_32(unsigned long size)
{
	return kmalloc(size, GFP_KERNEL);
}


void *vmalloc(unsigned long size)
{
	return kmalloc(size, GFP_KERNEL);
}

void vfree(void * p) {
	kfree(p);	
}

loff_t no_llseek(struct file *file, loff_t offset, int origin)
{
	return -ESPIPE;
}
