#ifdef L4_THREAD_SAFE

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <l4/crtx/ctor.h>
#include <l4/thread/thread.h>

fastcall int
l4uclibc_mutex_lock(pthread_mutex_t *mutex)
{
  l4thread_t self;

  switch (mutex->__m_kind)
    {
    case PTHREAD_MUTEX_ADAPTIVE_NP:
      l4semaphore_down(&mutex->__m_sem);
      return 0;

    case PTHREAD_MUTEX_RECURSIVE_NP:
      self = l4thread_myself();
      if (l4thread_equal(self, mutex->__m_owner))
	{
	  mutex->__m_count++;
	  return 0;
	}
      l4semaphore_down(&mutex->__m_sem);
      mutex->__m_owner = self;
      mutex->__m_count = 0;
      return 0;

    default:
      return EINVAL;
    }
}

fastcall int
l4uclibc_mutex_unlock(pthread_mutex_t *mutex)
{
  switch (mutex->__m_kind)
    {
    case PTHREAD_MUTEX_RECURSIVE_NP:
      if (!l4thread_equal(mutex->__m_owner, l4thread_myself()))
	return EPERM;
      if (mutex->__m_count > 0)
	{
	  mutex->__m_count--;
	  return 0;
	}
      mutex->__m_owner = L4THREAD_INVALID_ID;
      // fall through

    case PTHREAD_MUTEX_ADAPTIVE_NP:
      l4semaphore_up(&mutex->__m_sem);
      return 0;

    default:
      return EINVAL;
    }
}

fastcall int
l4uclibc_mutex_trylock(pthread_mutex_t *mutex)
{
  int ret;
  l4thread_t self;

  switch (mutex->__m_kind)
    {
    case PTHREAD_MUTEX_ADAPTIVE_NP:
      ret = l4semaphore_try_down(&mutex->__m_sem);
      return ret == 0 ? EBUSY : 0;
    case PTHREAD_MUTEX_RECURSIVE_NP:
      self = l4thread_myself();
      if (mutex->__m_owner == self)
	{
	  mutex->__m_count++;
	  return 0;
	}
      ret = l4semaphore_try_down(&mutex->__m_sem);
      if (ret == 1)
	{
	  mutex->__m_owner = self;
	  mutex->__m_count = 0;
	}
      return ret;

    default:
      return EINVAL;
    }
}

fastcall int
l4uclibc_mutex_init(pthread_mutex_t *mutex, const void *mutex_attr)
{
  mutex->__m_sem   = L4SEMAPHORE_UNLOCKED;
  mutex->__m_kind  = PTHREAD_MUTEX_ADAPTIVE_NP;
  mutex->__m_count = 0;
  mutex->__m_owner = L4THREAD_INVALID_ID;
  return 0;
}

static void
l4uclibc_mutex_initialize(void)
{
  FILE *fp;

  /* Enable locking. 
   * See also linuxthreads/pthread.c, pthread_initialize() */
  _stdio_user_locking = 0;       /* 2 if threading not initialized */
  for (fp = _stdio_openlist; fp != NULL; fp = fp->__nextopen)
    {
      if (fp->__user_locking != 1)
	fp->__user_locking = 0;
    }
}
L4C_CTOR(l4uclibc_mutex_initialize, L4CTOR_AFTER_BACKEND);

#endif /* L4_THREAD_SAFE */
