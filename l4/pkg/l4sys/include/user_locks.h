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

enum l4_u_semaphore_ret_t
{
  L4_USEM_OK      = 0, ///< OK, got the semaphore
  L4_USEM_RETRY   = 1, ///< never seen by user
  L4_USEM_TIMEOUT = 2, ///< The timeout has hit
  L4_USEM_INVALID = 3  ///< The Semaphore/Lock was destroyed
};

typedef struct l4_u_semaphore_t
{
  l4_mword_t counter;
  l4_umword_t flags;
} l4_u_semaphore_t;


L4_INLINE unsigned long
l4_ulock_generic(unsigned long op, unsigned long lock);


L4_INLINE unsigned long l4_ulock_new(unsigned long lock);
L4_INLINE unsigned long l4_ulock_lock(unsigned long lock);
L4_INLINE unsigned long l4_ulock_unlock(unsigned long lock);


L4_INLINE unsigned long l4_usem_new(unsigned long ksem, long count, 
    l4_u_semaphore_t *sem);

L4_INLINE unsigned
l4_usem_down_to(unsigned long ksem, l4_u_semaphore_t *sem,
    l4_timeout_s timeout);

L4_INLINE unsigned
l4_usem_down(unsigned long ksem, l4_u_semaphore_t *sem);

L4_INLINE void
l4_usem_up(unsigned long ksem, l4_u_semaphore_t *sem);



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
l4_usem_new(unsigned long ksem, long count, l4_u_semaphore_t *sem)
{ 
  unsigned long res = l4_ulock_generic(L4_USEM_NEW, ksem);
  if (res == 0)
    {
      sem->counter = count;
      sem->flags = 0;
    }

  return res;
}


L4_INLINE unsigned
l4_usem_down(unsigned long ksem, l4_u_semaphore_t *sem)
{
  return l4_usem_down_to(ksem, sem, L4_IPC_TIMEOUT_NEVER);
}

#endif
