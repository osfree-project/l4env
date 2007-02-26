#ifndef BITS_PTHREADTYPES_H
#define BITS_PTHREADTYPES_H

#include <l4/lock/lock.h>

typedef struct
{
  l4semaphore_t	__m_sem;
  l4thread_t	__m_owner;
  int		__m_count;
  int		__m_kind;
} pthread_mutex_t;

enum
{
  PTHREAD_MUTEX_ADAPTIVE_NP,
  PTHREAD_MUTEX_RECURSIVE_NP,
};

#endif
