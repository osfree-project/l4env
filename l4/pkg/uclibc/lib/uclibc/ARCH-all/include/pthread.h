#ifndef _PTHREAD_H
#define _PTHREAD_H

#include <l4/sys/compiler.h>
#include <bits/pthreadtypes.h>

/* Emulation of pthread_mutex functions necessary for making uClibc functions
 * thread-safe. */

#define __pthread_mutex_lock(x)		l4uclibc_mutex_lock(x)
#define __pthread_mutex_unlock(x)	l4uclibc_mutex_unlock(x)
#define __pthread_mutex_trylock(x)	l4uclibc_mutex_trylock(x)
#define __pthread_mutex_init(x,y)	l4uclibc_mutex_init(x,y)

fastcall int l4uclibc_mutex_lock(pthread_mutex_t *mutex);
fastcall int l4uclibc_mutex_unlock(pthread_mutex_t *mutex);
fastcall int l4uclibc_mutex_trylock(pthread_mutex_t *mutex);
fastcall int l4uclibc_mutex_init(pthread_mutex_t *mutex, const void *attr);

/* Initializer for ``fast'' mutex. Behavior like pure L4env semaphore: If
 * a thread attempts to lock a mutex it already owns, the calling thread
 * is simply suspended forever, thus effectively causing the calling thread
 * to deadlock (see also `man pthread_mutex_init'). */
#define PTHREAD_MUTEX_INITIALIZER		\
  {L4SEMAPHORE_UNLOCKED_INITIALIZER,		\
    (L4THREAD_INVALID_ID), 0,			\
    PTHREAD_MUTEX_ADAPTIVE_NP}

/* Initiaizer for ``recursive'' mutex. Behavior like L4env lock: If a thread
 * attempts to lock a mutex it already owns, pthread_mutex_lock succeeds and
 * returns immediately, recording the number of times the calling thread has
 * locked the mutex. An equal number of pthread_mutex_unlock operations must
 * be performed before the mutex returns to the unlocked state (see also 
 * `man pthread_mutex_init'). */
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP	\
  {L4SEMAPHORE_UNLOCKED_INITIALIZER,		\
   (L4THREAD_INVALID_ID), 0,			\
   PTHREAD_MUTEX_RECURSIVE_NP}

#endif
