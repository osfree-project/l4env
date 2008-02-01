#include "local.h"

#include <linux/kernel.h>

/**
 * DDE local IRQ handling
 *
 * Linux allows CPUs to locally disable and enable interrupts, e.g.
 * by setting the IF flag in X86' EFLAGS. In DDE, we can't allow to
 * do this globally, because we don't want our driver to influence
 * independent L4 tasks.
 *
 * Instead, we could simply leave the routines below empty, because
 * each DDE thread is a separate "virtual" CPU and therefore cannot
 * be interrupted. Global variables are secured by spinlocks, which
 * also work without IRQs disabled. However, DDE threads might run
 * in different priorities which could lead to a priority inversion
 * situation, where a high-prio thread spins on a spinlock held by
 * a low-prio thread. Therefore, we use a DDEKit lock to ensure that
 * all VCPUs that try to switch off IRQs locally are serialized.
 *
 * In addition to the lock, we need to keep track of the number of
 * times irq_disable() has been called. This is because it is quite
 * common for Linux drivers to call this multiple times in a row and
 * then only call irqs_enable() once. In our case we then need to
 * release the lock as many times as we acquired it, because the
 * underlying L4 lock implementation implements recursive locking.
 *
 * Furthermore, we need to be able to check whether IRQs are on or
 * off. Linux does this by having a look at the IF flag. In our case,
 * we return the current reference count during save_flags() and use
 * this value to restore the current state later on.
 */

/* The IRQ lock */
static ddekit_lock_t _irq_lock = NULL;
/* IRQ lock reference counter */
static atomic_t      _refcnt   = ATOMIC_INIT(0);

#define DDE26_DEBUG_IRQLOCK    0

/** Initialize the global IRQ lock. */
void l4dde26_init_locks(void)
{
	ddekit_lock_init_unlocked(&_irq_lock);
}
core_initcall(l4dde26_init_locks);

/* Check whether IRQs are currently disabled.
 *
 * This is the case, if flags is greater than 0.
 */

int raw_irqs_disabled_flags(unsigned long flags)
{
#if DDE26_DEBUG_IRQLOCK
	DEBUG_MSG("flags %x", flags);
#endif
	return ((int)flags > 0);
}

/* Store the current flags state.
 *
 * This is done by returning the current refcnt.
 *
 * XXX: Up to now, flags was always 0 at this point and
 *      I assume that this is always the case. Prove?
 */
unsigned long __raw_local_save_flags(void)
{
#if DDE26_DEBUG_IRQLOCK
	DEBUG_MSG("flags %x", atomic_read(&_refcnt));
#endif
	return (unsigned long)atomic_read(&_refcnt);
}

/* Restore IRQ state. */
void raw_local_irq_restore(unsigned long flags)
{
#if DDE26_DEBUG_IRQLOCK
	DEBUG_MSG("flags %d, refcount %d", (int)flags, atomic_read(&_refcnt));
#endif
	raw_local_irq_enable();
}

/* Disable IRQs by grabbing the IRQ lock. */
void raw_local_irq_disable(void)
{
#if DDE26_DEBUG_IRQLOCK
	DEBUG_MSG("owner %x", ddekit_lock_owner(&_irq_lock));
#endif

	ddekit_lock_lock(&_irq_lock);
	atomic_inc(&_refcnt);

#if DDE26_DEBUG_IRQLOCK
	DEBUG_MSG("count: %d, new owner %x", atomic_read(&_refcnt),
	          ddekit_lock_owner(&_irq_lock));
#endif
}

/* Unlock the IRQ lock until refcnt is 0. */
void raw_local_irq_enable(void)
{
	int i = atomic_read(&_refcnt);
#if DDE26_DEBUG_IRQLOCK
	DEBUG_MSG("owner %x, count: %d",
	          ddekit_lock_owner(&_irq_lock), atomic_read(&_refcnt));
#endif
	for (i; i > 0; --i) {
		atomic_dec(&_refcnt);
		ddekit_lock_unlock(&_irq_lock);
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
