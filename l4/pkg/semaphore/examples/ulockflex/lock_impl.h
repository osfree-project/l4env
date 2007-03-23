#ifndef LOCK_IMPL_H
#define LOCK_IMPL_H

#ifdef LOCAL_SEM
#include <l4/semaphore/semaphore.h>

#define LOCK_TYPE l4semaphore_t
#define LOCK_SIZE sizeof(l4semaphore_t)
#define LOCK_DOWN(sem) l4semaphore_down(sem)
#define LOCK_UP(sem) l4semaphore_up(sem)
#define LOCK_INIT(sem) *sem = L4SEMAPHORE_INIT(1)

#elif L4UTIL_LOCK
#include <l4/util/lock.h>

#define LOCK_TYPE l4util_simple_lock_t
#define LOCK_SIZE sizeof(l4util_simple_lock_t)
#define LOCK_INIT(lock) l4_simple_unlock(lock)
#define LOCK_DOWN(lock) l4_simple_lock(lock)
#define LOCK_UP(lock) l4_simple_unlock(lock)
#endif

#ifndef LOCK_TYPE
#error LOCK_* defines are not set
#endif

#endif
