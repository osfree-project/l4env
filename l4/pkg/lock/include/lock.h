/* $Id$ */
/*****************************************************************************/
/**
 * \file  lock/include/l4/lock.h
 * \brief Simple L4 locks.
 *
 * \date   12/30/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Simple, semaphore-based lock implementation.
 */
/*****************************************************************************/
#ifndef _L4_LOCK_LOCK_H
#define _L4_LOCK_LOCK_H

/* L4 includes */
#include <l4/env/cdefs.h>
#include <l4/semaphore/semaphore.h>

/* Lib includes */
#include <l4/lock/types.h>

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

__BEGIN_DECLS;

/*****************************************************************************/
/**
 * \brief   Occupy lock
 * \ingroup api
 * 
 * \param   lock         Lock structure
 *
 * Occupy lock, block if lock already owned by someone else.
 */
/*****************************************************************************/ 
volatile L4_INLINE void
l4lock_lock(l4lock_t * lock);

/*****************************************************************************/
/**
 * \brief   Try to occupy lock, return error if lock already owned by someone 
 *          else.
 * \ingroup api
 * 
 * \param   lock         Lock structure
 * \return  1 on success (occupied lock or lack already owned by calling 
 *          thread), 0 if lock owned by someone else
 */
/*****************************************************************************/ 
volatile L4_INLINE int
l4lock_try_lock(l4lock_t * lock);

/*****************************************************************************/
/**
 * \brief   Release lock.
 * \ingroup api
 * 
 * \param   lock         Lock structure.
 */
/*****************************************************************************/ 
volatile L4_INLINE void
l4lock_unlock(l4lock_t * lock);

/*****************************************************************************/
/**
 * \brief   Return lock owner
 * \ingroup api
 * 
 * \param   lock         Lock structure
 *	
 * \return  Lock owner, #L4THREAD_INVALID_ID if no one owns the lock
 */
/*****************************************************************************/ 
L4_INLINE l4thread_t
l4lock_owner(l4lock_t * lock);

__END_DECLS;

/*****************************************************************************
 *** implementation
 *****************************************************************************/

/*****************************************************************************
 * lock
 *****************************************************************************/
volatile L4_INLINE void
l4lock_lock(l4lock_t * lock)
{
  l4thread_t me = l4thread_myself();

  /* sanity check */
  if (lock == NULL)
    return;

  /* reenter? */
  if (l4thread_equal(me,lock->owner))
    {
      lock->ref_count++;
      return;
    }
  
  /* get semaphore */
  l4semaphore_down(&lock->sem);

  /* set owner / reference counter */
  lock->owner = me;
  lock->ref_count = 1;
}

/*****************************************************************************
 * try lock
 *****************************************************************************/
volatile L4_INLINE int
l4lock_try_lock(l4lock_t * lock)
{
  l4thread_t me = l4thread_myself();

  /* sanity check */
  if (lock == NULL)
    return 0;

  /* reenter? */
  if (l4thread_equal(me,lock->owner))
    {
      lock->ref_count++;
      return 1;
    }
  
  /* try to get semaphore */
  if (l4semaphore_try_down(&lock->sem))
    {
      /* got semaphore */
      lock->owner = me;
      lock->ref_count = 1;
      
      return 1;
    }
  else
    /* semaphore already locked */
    return 0;
}

/*****************************************************************************
 * unlock
 *****************************************************************************/
volatile L4_INLINE void
l4lock_unlock(l4lock_t * lock)
{
  l4thread_t me = l4thread_myself();

  /* sanity checks */
  if (lock == NULL)
    return;

  if (!l4thread_equal(lock->owner,me))
    return;

  /* decrement reference counter */
  lock->ref_count--;
  if (lock->ref_count <= 0)
    {
      /* release lock */
      lock->owner = L4THREAD_INVALID_ID;
      l4semaphore_up(&lock->sem);
    }
}

/*****************************************************************************
 * owner
 *****************************************************************************/
L4_INLINE l4thread_t
l4lock_owner(l4lock_t * lock)
{
  /* sanity checks */
  if (lock == NULL)
    return L4THREAD_INVALID_ID;

  /* return owner */
  return lock->owner;
}

#endif /* _L4_LOCK_LOCK_H */
