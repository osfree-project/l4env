#ifndef __LINUX_SPINLOCK_H
#define __LINUX_SPINLOCK_H

#include <l4/lock/lock.h>

typedef l4lock_t spinlock_t;

#define SPIN_LOCK_UNLOCKED ((l4lock_t)L4LOCK_UNLOCKED_INITIALIZER)
#define DEFINE_SPINLOCK(x) spinlock_t x = SPIN_LOCK_UNLOCKED

static inline void spin_unlock(spinlock_t *lock)
{
	l4lock_unlock(lock);
}

static inline void spin_lock(spinlock_t *lock)
{
	l4lock_lock(lock);
}

#define spin_lock_init(x)                   \
	do {                                \
	    *(x) = SPIN_LOCK_UNLOCKED;       \
	} while (0)

/* XXX lock irq threads here??? */

#define spin_lock_irqsave(lock, flags)      \
	do {                                \
                (void)flags;                \
		spin_lock(lock);            \
	} while (0)
#define spin_unlock_irqrestore(lock, flags) \
	do {                                \
                (void)flags;                \
		spin_unlock(lock);          \
	} while (0)

#define spin_lock_irq(lock)                 \
	do {                                \
		spin_lock(lock);            \
	} while (0)

#define spin_unlock_irq(lock)               \
	do {                                \
		spin_unlock(lock);          \
	} while (0)

#endif

