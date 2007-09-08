#include "local.h"

#include <linux/kernel.h>

static ddekit_lock_t _irq_lock = NULL;

/** Initialize the global IRQ lock. */
void l4dde26_init_locks(void)
{
	ddekit_lock_init(&_irq_lock);
}
core_initcall(l4dde26_init_locks);

unsigned long __raw_local_save_flags(void)
{
	return 0;
}

void raw_local_irq_restore(unsigned long flags)
{
    /* flags = raw_local_restore_flags(); */
	raw_local_irq_enable();
}

void raw_local_irq_disable(void)
{
	ddekit_lock_lock(&_irq_lock);
}

void raw_local_irq_enable(void)
{
	ddekit_lock_unlock(&_irq_lock);
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
