#ifndef _I386_SEMAPHORE_H
#define _I386_SEMAPHORE_H

#include <l4/lock/lock.h>
#include <linux/wait.h>

/* XXX Semaphores are used only for mutual exclusion in input for now. Given
 * this fact and because "kseriod" is not desirable we use L4 locks that can
 * be reentered by one and the same thread. */

struct semaphore {
	l4lock_t l4_lock;
};

#define DECLARE_MUTEX(name)		\
	struct semaphore name = {l4_lock: L4LOCK_UNLOCKED}
#define DECLARE_MUTEX_LOCKED(name)	\
	struct semaphore name = {l4_lock: L4LOCK_LOCKED}

static inline void init_MUTEX (struct semaphore * sem)
{
	sem->l4_lock = L4LOCK_UNLOCKED;
}

static inline void down(struct semaphore * sem)
{
	l4lock_lock(&sem->l4_lock);
}

static inline void up(struct semaphore * sem)
{
	l4lock_unlock(&sem->l4_lock);
}

#endif

