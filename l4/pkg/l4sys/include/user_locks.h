#ifndef L4SYS_USER_LOCKS_H__
#define L4SYS_USER_LOCKS_H__

#include <l4/sys/types.h>

/*****************************************************************************
 *** Prototypes
 *****************************************************************************/

enum
{
  L4_ULOCK_NEW = 1,
  L4_ULOCK_LOCK = 3,
  L4_ULOCK_UNLOCK = 4,
  L4_USEM_NEW = 5,
  L4_USEM_SLEEP = 6,
  L4_USEM_WAKEUP = 7,
};

L4_INLINE unsigned long
l4_ulock_generic(unsigned long op, unsigned long lock);


L4_INLINE unsigned long l4_ulock_new(unsigned long lock);
L4_INLINE unsigned long l4_ulock_lock(unsigned long lock);
L4_INLINE unsigned long l4_ulock_unlock(unsigned long lock);


L4_INLINE unsigned long l4_usem_new(unsigned long sem);

L4_INLINE unsigned
l4_usem_down_to(unsigned long lock, unsigned long *counter, 
                l4_timeout_s timeout);

L4_INLINE unsigned
l4_usem_down(unsigned long lock, unsigned long *counter);

L4_INLINE void
l4_usem_up(unsigned long lock, unsigned long *counter);



/*****
 * Implementations
 */
#include <l4/sys/__user_locks_impl.h>

L4_INLINE unsigned long
l4_ulock_new(unsigned long lock)
{ return l4_ulock_generic(L4_ULOCK_NEW, lock); }


L4_INLINE unsigned long
l4_ulock_lock(unsigned long lock)
{ return l4_ulock_generic(L4_ULOCK_LOCK, lock); }


L4_INLINE unsigned long
l4_ulock_unlock(unsigned long lock)
{ return l4_ulock_generic(L4_ULOCK_UNLOCK, lock); }


L4_INLINE unsigned long
l4_usem_new(unsigned long sem)
{ return l4_ulock_generic(L4_USEM_NEW, sem); }


L4_INLINE unsigned
l4_usem_down(unsigned long lock, unsigned long *counter)
{
  return l4_usem_down_to(lock, counter, L4_IPC_TIMEOUT_NEVER);
}

#endif
