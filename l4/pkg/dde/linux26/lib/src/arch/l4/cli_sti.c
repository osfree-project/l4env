#include "local.h"

#include <linux/kernel.h>

static ddekit_lock_t _irq_lock = NULL;
// counter of threads with currently disabled irqs
static atomic_t _irq_counter = ATOMIC_INIT(0);

/** Initialize the global IRQ lock. */
void l4dde26_init_locks(void)
{
	ddekit_lock_init_unlocked(&_irq_lock);
}
core_initcall(l4dde26_init_locks);


/* 
 * Check whether IRQs are disabled by checking no. of IRQ disabled
 * threads.
 */
int raw_irqs_disabled_flags(unsigned long flags)
{
	return atomic_read(&_irq_counter) != 0;
}


/*
 * This fn is used to return the current state of IRQ flags. In our case
 * we return the local IRQ counter
 */
unsigned long __raw_local_save_flags(void)
{
	unsigned long old = atomic_read(&_irq_counter);
	return old;
}


/*
 * Restore previous IRQ state. If the prev. state was higher than the
 * current count, we need to call disable(), otherwise we need to call
 * enable().
 */
void raw_local_irq_restore(unsigned long flags)
{
	if (flags > atomic_read(&_irq_counter))
		raw_local_irq_disable();
	else if (flags < atomic_read(&_irq_counter))
		raw_local_irq_enable();
}


void raw_local_irq_disable(void)
{
	atomic_inc(&_irq_counter);
	ddekit_lock_lock(&_irq_lock);
}


void raw_local_irq_enable(void)
{
	/*
	 * Make sure we only unlock if there is anything locked.
	 */
	if (atomic_read(&_irq_counter) > 0) {
		ddekit_lock_unlock(&_irq_lock);
		atomic_dec(&_irq_counter);
	}
}


void raw_safe_halt(void)
{
	WARN_UNIMPL;
}


void halt(void)
{
	WARN_UNIMPL;
}

/* These functions are empty for DDE. Every DDE thread is a separate
 * "virtual" CPU. Therefore there is no need to en/disable bottom halves.
 */
void local_bh_disable(void) {}
void __local_bh_enable(void) {}
void _local_bh_enable(void) {}
void local_bh_enable(void) {}
void local_bh_enable_ip(unsigned long ip) {}
