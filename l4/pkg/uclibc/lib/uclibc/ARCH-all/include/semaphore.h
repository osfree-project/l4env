#ifndef _UCLIBC_SEMAPHORE_H
#define _UCLIBC_SEMAPHORE_H 1

#include <l4/semaphore/semaphore.h>
#include <errno.h>

typedef l4semaphore_t sem_t;

__BEGIN_DECLS

inline int sem_init (sem_t *sem, int shared, unsigned int value)
{
	if (shared) {
		assert(!"Sharing semaphores between processes in L4Env is not supported");
		return -1;
	}
	if (sem == NULL)
		return -1;
	
	sem->counter = value;  /**< semaphore counter */
	sem->pending = 0;        /**< wakeup notification pending counter */
	sem->queue   = NULL;     /**< wait queue */

	return 0;
}

inline int sem_destroy (sem_t *sem) {
	return 0;
}

extern sem_t *sem_open (const char *name, int oflag, ...); 

extern int sem_close (sem_t *sem);

extern int sem_unlink (const char *name);

inline int sem_wait (sem_t *sem) {
  l4semaphore_down(sem);
  return 0;
}

inline int sem_trywait (sem_t *sem) {
  if (!l4semaphore_try_down(sem))
	return EAGAIN;
  else
	return 0;
}

inline int sem_post (sem_t *sem) {
  l4semaphore_up(sem);
  return 0;
}

extern int sem_getvalue (sem_t * sem, int * sval);

__END_DECLS

#endif
